#include "esp_log.h"
#include "esp_err.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "rflink_cc1101_spi.hpp"
#include "rflink_packet_rx.hpp"
#include "rflink_radio_manager.hpp"
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
namespace {
static std::atomic<uint32_t> rx_worker_cycles{0};
static uint32_t boot_uptime_ms(){return static_cast<uint32_t>(esp_timer_get_time()/1000);}
static rflink::PacketRx868* rx868=nullptr;
static rflink::OokPulseReceiver* ook433a=nullptr;
static rflink::OokPulseReceiver* ook433b=nullptr;
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
  while(true){
    if(rx868)rx868->poll_once();
    if(ook433a)ook433a->poll_once();
    if(ook433b)ook433b->poll_once();
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
  for(unsigned i=0;i<3;i++)manager.set_presence(static_cast<rflink::RadioId>(i),radio_detected[i],radio_detected[i]);
  const auto rx868_error=manager.start_receive(rflink::RadioId::rf86830,rflink::RfProfileId::itho_868_packet);
  const auto rx433a_error=manager.start_receive(rflink::RadioId::rf43342,rflink::RfProfileId::ook_43342_pulses);
  const auto rx433b_error=manager.start_receive(rflink::RadioId::rf43392,rflink::RfProfileId::ook_43392_pulses);
  if(rx868_error==rflink::ManagerError::none)rx868=&packet_rx;
  if(rx433a_error==rflink::ManagerError::none)ook433a=&ook_a;
  if(rx433b_error==rflink::ManagerError::none)ook433b=&ook_b;
  ESP_LOGW("rflink","RX start: 43342=%u 43392=%u 86830=%u (0=OK)",
     unsigned(rx433a_error),unsigned(rx433b_error),unsigned(rx868_error));
  bool rx_worker_started=false;
  if(rx868 || ook433a || ook433b){
    rx_worker_started=xTaskCreate(radio_rx_task,"rf_rx",8192,nullptr,5,nullptr)==pdPASS;
    if(!rx_worker_started)
      ESP_LOGE("rflink","RX worker creation failed; capture polling inactive");
  }
  rflink_set_manager_provider(&manager);
  rflink_set_radio_snapshot(spi_ok, radio_detected);
  rflink_set_spi_stats_provider(&bus);
  rflink_set_rx_stats_provider(&packet_rx);
  rflink_set_decoder_stats_provider(&decoder);
  rflink_set_rx_worker_cycles_provider(&rx_worker_cycles);
  rflink_set_ook_stats_providers(&rmt_a,&rmt_b);
  ESP_LOGW("rflink","RADIOACTIVE STAGE 20 BENCH: UART/RMT/FIFO TX transports wired to manager but NO UI/API trigger; 433 raw RX; do not replace v0.6.6");
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
