#include "esp_log.h"
#include "esp_err.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "rflink_cc1101_spi.hpp"
#include "rflink_packet_rx.hpp"
#include "rflink_radio_manager.hpp"
#include "rflink_rx_watchdog.hpp"
#include "rflink_rmt_rx.hpp"
#include "rflink_ook_rx.hpp"
#include "rflink_native_decoder.hpp"
#include "rflink_uart_tx.hpp"
#include "rflink_rmt_tx.hpp"
#include "rflink_packet_tx.hpp"
#include "rflink_native_safety.hpp"
#include <atomic>
#include "freertos/task.h"
#include "rflink_web_ota.hpp"
#include "rflink_mqtt.hpp"
#include "rflink_crash_report.hpp"
namespace {
static std::atomic<uint32_t> rx_worker_cycles{0};
static uint32_t boot_uptime_ms(){return static_cast<uint32_t>(esp_timer_get_time()/1000);}
static rflink::PacketRx868* rx868=nullptr;
static rflink::OokPulseReceiver* ook433a=nullptr;
static rflink::OokPulseReceiver* ook433b=nullptr;
static rflink::RadioManager* rx_manager=nullptr;
static rflink::Cc1101SpiBus* rx_bus=nullptr;
// A radio may appear after boot (bad contact or late power), or a running
// RMT/FIFO RX path may stop after an error. Check every five seconds, never
// reset a radio currently managed for TX/switching.
constexpr unsigned kRxRecoveryCheckLoops=1000;
static uint32_t rx_recovery_attempts[3]={};
static uint32_t rx_recovery_successes[3]={};
static rflink::RfProfileId receive_profile(rflink::RadioId id){
  return id==rflink::RadioId::rf86830
    ? rflink::RfProfileId::itho_868_packet
    : id==rflink::RadioId::rf43342
      ? rflink::RfProfileId::ook_43342_pulses
      : rflink::RfProfileId::ook_43392_pulses;
}
static bool receiver_listening(rflink::RadioId id){
  return id==rflink::RadioId::rf86830
    ? (rx868 && rx868->stats().listening)
    : id==rflink::RadioId::rf43342
      ? (ook433a && ook433a->stats().listening)
      : (ook433b && ook433b->stats().listening);
}
static void check_rx_recovery(rflink::RadioId radio) {
  if(!rx_manager || !rx_bus)return;
  const auto i=static_cast<unsigned>(radio);
  const auto state=rx_manager->status(radio);
  const auto action=rflink::rx_watchdog_action(state,receiver_listening(radio));
  if(action==rflink::RxWatchdogAction::none)return;
  ++rx_recovery_attempts[i];
  if(action==rflink::RxWatchdogAction::stop_then_restart) {
    // Never reset a CC1101 that may be transmitting. stop_receive performs
    // the same manager-serialized operation as an ordinary RX->TX handover.
    const auto stop=rx_manager->stop_receive(radio);
    if(stop!=rflink::ManagerError::none){
      ESP_LOGW("rflink.rxwatch","radio%u stop for recovery failed=%u",i,unsigned(stop));
      return;
    }
  }
  if(action==rflink::RxWatchdogAction::probe_then_start) {
    // Read only PARTNUM/VERSION. Hardware reset (SRES) is boot-only.
    const auto probed=rx_bus->probe(radio);
    if(!probed.plausible){
      if(rx_recovery_attempts[i]%12==1)
        ESP_LOGW("rflink.rxwatch","radio%u offline: err=%s part=%02x version=%02x",
          i,esp_err_to_name(probed.error),probed.partnum,probed.version);
      return;
    }
    rx_manager->set_presence(radio,true,true);
  }
  const auto started=rx_manager->start_receive(radio,receive_profile(radio));
  if(started==rflink::ManagerError::none){
    ++rx_recovery_successes[i];
    ESP_LOGI("rflink.rxwatch","radio%u RX recovered; attempt=%lu successes=%lu",
      i,static_cast<unsigned long>(rx_recovery_attempts[i]),
      static_cast<unsigned long>(rx_recovery_successes[i]));
  }else if(rx_recovery_attempts[i]%12==1){
    ESP_LOGW("rflink.rxwatch","radio%u RX restart failed=%u after %lu attempts",
      i,unsigned(started),static_cast<unsigned long>(rx_recovery_attempts[i]));
  }
}
class NativePacketTransport final: public rflink::RadioTransport {
public:
  explicit NativePacketTransport(rflink::PacketRx868& rx, rflink::OokPulseReceiver& a, rflink::OokPulseReceiver& b, rflink::Cc1101GdoUartTx& tx,
    rflink::Cc1101GdoRmtTx& rmt_tx,rflink::Cc1101PacketTx& packet_tx):
    rx_(rx),a_(a),b_(b),tx_(tx),rmt_tx_(rmt_tx),packet_tx_(packet_tx){}
  bool start_packet_rx(rflink::RadioId id,const rflink::RfProfile& p) override {
    return id==rflink::RadioId::rf86830 && p.id==rflink::RfProfileId::itho_868_packet && rx_.start()==ESP_OK;
  }
  bool stop_packet_rx(rflink::RadioId id) override { return id==rflink::RadioId::rf86830 && rx_.stop()==ESP_OK; }
  bool start_pulse_rx(rflink::RadioId id,const rflink::RfProfile& p) override {
    if (id==rflink::RadioId::rf43342) return a_.start(p)==ESP_OK;
    if (id==rflink::RadioId::rf43392) return b_.start(p)==ESP_OK;
    return false;
  }
  bool stop_pulse_rx(rflink::RadioId id) override {
    if (id==rflink::RadioId::rf43342)return a_.stop()==ESP_OK;
    if (id==rflink::RadioId::rf43392)return b_.stop()==ESP_OK;
    return false;
  }
  rflink::PhysicalTxResult transmit_uart(rflink::RadioId id,const rflink::RfProfile& p,const rflink::EncodedTxFrame& f) override {
    return tx_.transmit(id,p,f);
  }
  rflink::PhysicalTxResult transmit_rmt(rflink::RadioId id,const rflink::RfProfile& p,const rflink::EncodedTxFrame& f) override {
    return rmt_tx_.transmit(id,p,f);
  }
  rflink::PhysicalTxResult transmit_packet(rflink::RadioId id,const rflink::RfProfile& p,const rflink::EncodedTxFrame& f) override {
    return packet_tx_.transmit(id,p,f);
  }
private:rflink::PacketRx868& rx_; rflink::OokPulseReceiver& a_; rflink::OokPulseReceiver& b_;
  rflink::Cc1101GdoUartTx& tx_; rflink::Cc1101GdoRmtTx& rmt_tx_;rflink::Cc1101PacketTx& packet_tx_;
};
void radio_rx_task(void*) {
  unsigned loops=0;
  while(true){
    if(rx868)rx868->poll_once();
    if(ook433a)ook433a->poll_once();
    if(ook433b)ook433b->poll_once();
    if(++loops>=kRxRecoveryCheckLoops){
      loops=0;
      for(unsigned i=0;i<3;i++)
        check_rx_recovery(static_cast<rflink::RadioId>(i));
    }
    rx_worker_cycles.fetch_add(1,std::memory_order_relaxed);
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}
}
extern "C" void app_main(void) {
  esp_err_t e=nvs_flash_init();
  if(e==ESP_ERR_NVS_NO_FREE_PAGES || e==ESP_ERR_NVS_NEW_VERSION_FOUND){
    ESP_LOGE("rflink", "NVS incompatible; refusing to erase stored data");return;
  }
  if(e!=ESP_OK){ESP_LOGE("rflink","NVS init failed: %s",esp_err_to_name(e));return;}
  // Persist reboot reason and last valid crash PC/task before touching RF hardware.
  rflink::crash_report_record_boot();
  static rflink::Cc1101SpiBus bus;
  e=bus.begin();
  bool spi_ok=(e==ESP_OK);
  bool radio_detected[3]={false,false,false};
  if(e==ESP_OK){
    for(int i=0;i<3;i++){
      auto p=bus.boot_reset_and_probe(static_cast<rflink::RadioId>(i));
      radio_detected[i]=p.plausible;
      ESP_LOGI("rflink","SPI radio%d presence=%d part=0x%02x version=0x%02x marc=0x%02x error=%s",i,p.plausible,p.partnum,p.version,p.marcstate,esp_err_to_name(p.error));
    }
  }else ESP_LOGE("rflink","SPI init: %s",esp_err_to_name(e));
  static rflink::NativeDecoder decoder;
  decoder.set_uptime_provider(&boot_uptime_ms);
  static rflink::PacketRx868 packet_rx(bus);
  packet_rx.set_frame_sink(&decoder);
  static rflink::RmtPulseRx rmt_a;
  static rflink::RmtPulseRx rmt_b;
  rmt_a.set_frame_sink(&decoder);
  rmt_b.set_frame_sink(&decoder);
  static rflink::OokPulseReceiver ook_a(bus,rmt_a,rflink::RadioId::rf43342);
  static rflink::OokPulseReceiver ook_b(bus,rmt_b,rflink::RadioId::rf43392);
  static rflink::Cc1101GdoUartTx uart_tx(bus);
  static rflink::Cc1101GdoRmtTx rmt_tx(bus);
  static rflink::Cc1101PacketTx packet_tx(bus);
  static NativePacketTransport transport(packet_rx,ook_a,ook_b,uart_tx,rmt_tx,packet_tx);
  static rflink::RadioManager manager(transport);
  rx_bus=&bus;rx_manager=&manager;
  // Keep all receive objects addressable even if their initial start failed:
  // watchdog can detect and recover a late/unready module in place.
  rx868=&packet_rx;ook433a=&ook_a;ook433b=&ook_b;
  for(unsigned i=0;i<3;i++)manager.set_presence(static_cast<rflink::RadioId>(i),radio_detected[i],radio_detected[i]);
  const auto rx868_error=manager.start_receive(rflink::RadioId::rf86830,rflink::RfProfileId::itho_868_packet);
  const auto rx433a_error=manager.start_receive(rflink::RadioId::rf43342,rflink::RfProfileId::ook_43342_pulses);
  const auto rx433b_error=manager.start_receive(rflink::RadioId::rf43392,rflink::RfProfileId::ook_43392_pulses);
  ESP_LOGW("rflink","RX start: 43342=%u 43392=%u 86830=%u (0=OK)",
     unsigned(rx433a_error),unsigned(rx433b_error),unsigned(rx868_error));
  bool rx_worker_started=false;
  if(spi_ok){
    // Important: start even when ALL three initial RX starts failed. Otherwise
    // no background task exists to recover missing/late CC1101 modules.
    rx_worker_started=xTaskCreate(radio_rx_task,"rf_rx",8192,nullptr,5,nullptr)==pdPASS;
    if(!rx_worker_started)
      ESP_LOGE("rflink","RX worker creation failed; capture polling and recovery inactive");
  }
  rflink_set_manager_provider(&manager);
  rflink_set_radio_snapshot(spi_ok, radio_detected);
  rflink_set_spi_stats_provider(&bus);
  rflink_set_rx_stats_provider(&packet_rx);
  rflink_set_decoder_stats_provider(&decoder);
  rflink_set_rx_worker_cycles_provider(&rx_worker_cycles);
  rflink_set_ook_stats_providers(&rmt_a,&rmt_b);
  ESP_LOGW("rflink","RADIOACTIVE STAGE 21 BENCH: bounded RX recovery enabled; TX transports still have NO UI/API trigger; do not replace v0.6.6");
  bool web_ok=rflink_start_web_ota();
  if(web_ok)rflink::mqtt_service_init();
  // Confirm an OTA only if web + SPI + at least one actual RX worker started.
  // Stabilization delay catches early task crashes; it is still NOT proof of RF
  // on-air performance or that the entire v0.6.6 feature set works.
  // WiFi connectivity itself is NOT required (AP/router may temporarily be down).
  const esp_partition_t* running=esp_ota_get_running_partition();
  esp_ota_img_states_t state{};
  if (running && esp_ota_get_state_partition(running,&state)==ESP_OK &&
      state==ESP_OTA_IMG_PENDING_VERIFY) {
    const uint32_t before=rx_worker_cycles.load(std::memory_order_relaxed);
    vTaskDelay(pdMS_TO_TICKS(10000));
    const uint32_t cycles=rx_worker_cycles.load(std::memory_order_relaxed)-before;
    // A started task alone is insufficient. Also check live RX state and
    // CC1101 SPI responsiveness without resetting or changing RF registers.
    unsigned active_rx=0,responsive=0;
    for(unsigned i=0;i<3;i++) {
      const auto radio=static_cast<rflink::RadioId>(i);
      if(!radio_detected[i])continue;
      const auto st=manager.status(radio);
      const bool listening=(i==0 ? rmt_a.stats().listening :
                            i==1 ? rmt_b.stats().listening : packet_rx.stats().listening);
      if(st.operation==rflink::RadioOperation::receiving && listening) ++active_rx;
      uint8_t part=0xff,version=0xff;
      if(bus.read_identity(radio,&part,&version)==ESP_OK &&
         part==0x00 && version!=0x00 && version!=0xff) ++responsive;
    }
    rflink::BootHealth health{spi_ok,web_ok,rx_worker_started,cycles,active_rx,responsive};
    if(rflink::passes_boot_health(health)) {
      const auto result=esp_ota_mark_app_valid_cancel_rollback();
      if(result==ESP_OK)
        ESP_LOGI("rflink","OTA validated: task loops=%lu active_RX=%u responsive_SPI=%u",
          static_cast<unsigned long>(cycles),active_rx,responsive);
      else
        ESP_LOGE("rflink","OTA validation mark failed: %s",esp_err_to_name(result));
    } else {
      ESP_LOGE("rflink","OTA health failed: spi=%d web=%d worker=%d cycles=%lu active_RX=%u responsive_SPI=%u; rollback",
        int(spi_ok),int(web_ok),int(rx_worker_started),
        static_cast<unsigned long>(cycles),active_rx,responsive);
      const auto rollback=esp_ota_mark_app_invalid_rollback_and_reboot();
      ESP_LOGE("rflink","OTA rollback request returned: %s",esp_err_to_name(rollback));
    }
  }
}
