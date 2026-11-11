#pragma once
// Universal CC1101 asynchronous GDO0 -> MCU UART TX physical adapter.
// No protocol names, no pairing code. A profile supplies vetted modem registers
// and UART baud rate; decoder encoder supplies already-framed wire bytes.
// Default: NOT instantiated, RF transmissions impossible.
#ifdef USE_ESP32
#include <array>
#include <cstring>
#include <cstdio>
#include <string>
#include "gdo_serial_tx_session.h"
#include "serial_tx_adapter.h"
#include "radio_mode_manager.h"
#include "cc1101_packet_rx_restore.h"
#include "esphome/core/hal.h"
#include "ramses_spi_esp_backend.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esphome/core/hal.h"

namespace esphome { namespace rflink_universal {
struct GdoUartPreset {
  const char *name;
  uint32_t baud;
  uint8_t pktctrl0; // CC1101 asynchronous serial mode, no HW FIFO/CRC
  uint8_t mdmcfg4, mdmcfg3, mdmcfg2, deviatn;
};
class CC1101GdoUartTx final : public SerialTxAdapter, private GdoSerialTxOperations {
 public:
  CC1101GdoUartTx(cc1101::CC1101Component &radio,
                  RadioModeManager &manager, uint8_t channel, int gdo0_gpio,
                  GdoUartPreset preset, bool explicit_lab_enable)
      : radio_(radio), bus_(radio), mode_(manager), channel_(channel),
        pin_(gdo0_gpio), preset_(preset), enabled_(explicit_lab_enable), session_(*this) {}
  bool ready(uint8_t channel, const char *name) const override {
    return enabled_ && session_.ready() && !active_ &&
           channel == channel_ && name && preset_.name && std::strcmp(name,preset_.name)==0 &&
           channel < 3 && valid_tx_pin_(pin_) && !radio_.is_failed() && mode_.itho_ready() &&
           preset_.baud >= 1200 && preset_.baud <= 115200 && preset_.pktctrl0 == 0x30 &&
           !uart_is_driver_installed(UART_NUM_1);
  }
  bool send(uint8_t channel, const char *preset_name, const uint8_t *wire, size_t count,
            uint8_t repeats, uint16_t gap, std::string &error) override {
    if (!ready(channel,preset_name)) {
      if (!enabled_) error="GDO/UART TX disabled by explicit lab gate";
      else if (!session_.ready()) error="GDO/UART TX blocked after previous RX recovery failure";
      else if (active_) error="GDO/UART radio already acquired";
      else if (channel != channel_ || !preset_name || !preset_.name ||
               std::strcmp(preset_name,preset_.name)!=0)
        error="GDO/UART channel or approved profile mismatch";
      else if (channel >= 3 || !valid_tx_pin_(pin_)) error="GDO/UART invalid radio channel or TX GPIO";
      else if (radio_.is_failed()) error="GDO/UART CC1101 has hardware failure";
      else if (!mode_.itho_ready()) error="GDO/UART shared 868 MHz RX lease is not available";
      else if (uart_is_driver_installed(UART_NUM_1)) error="GDO/UART UART1 already occupied";
      else error="GDO/UART baud or async modem preset invalid";
      std::snprintf(failure_,sizeof(failure_),"%s",error.c_str());
      trace_stage("GDO/UART TX readiness",false);
      return false;
    }
    return session_.run(wire,count,repeats,gap,error);
  }
  bool sticky_fault() const { return session_.fault(); }
  const char *last_failure() const { return failure_; }
  using TraceObserver = void (*)(void *ctx,const char *stage,bool success);
  void set_trace_observer(TraceObserver observer,void *ctx){observer_=observer;context_=ctx;}
  // No silent fault clearing. A separate board diagnostic/reboot is required.
 private:
  const char *failure_detail() const override { return failure_; }
  bool fail_step_(const char *step) {
    std::snprintf(failure_,sizeof(failure_),"%s failed",step);
    trace_stage(step,false);
    return false;
  }
  bool fail_esp_(const char *step,esp_err_t result) {
    std::snprintf(failure_,sizeof(failure_),"%s: %s (0x%X)",
                  step,esp_err_to_name(result),static_cast<unsigned>(result));
    trace_stage(step,false);
    return false;
  }
  bool check_esp_(const char *step,esp_err_t result) {
    if(result!=ESP_OK)return fail_esp_(step,result);
    trace_stage(step,true);
    return true;
  }
  bool fail_register_(uint8_t reg,uint8_t expected,uint8_t found,bool read_ok) {
    if (!read_ok)
      std::snprintf(failure_,sizeof(failure_),"CC1101 readback failed at register 0x%02X",reg);
    else
      std::snprintf(failure_,sizeof(failure_),
        "CC1101 register 0x%02X readback mismatch: expected 0x%02X, got 0x%02X",
        reg,expected,found);
    trace_stage("CC1101 register verification",false);
    return false;
  }
  void trace_stage(const char *stage,bool success) override {
    if(observer_) observer_(context_,stage,success);
  }
  static bool valid_tx_pin_(int gpio) {
    // S3 GPIO 46 is input-only; reserved strapping pins excluded deliberately.
    return (gpio >= 0 && gpio <= 21) || (gpio >= 26 && gpio <= 44) || gpio == 47 || gpio == 48;
  }
  bool acquire() override {
    failure_[0]='\0';
    if (active_) return fail_step_("CC1101 radio lease already active");
    if (!mode_.itho_ready())return fail_step_("CC1101 RX lease unavailable");
    if (radio_.is_failed())return fail_step_("CC1101 radio previously failed");
    if (uart_is_driver_installed(UART_NUM_1))return fail_step_("UART1 already occupied");
    for (size_t i=0;i<registers_.size();++i) {
      if (!bus_.read(registers_[i], snapshot_[i])) {
        std::snprintf(failure_,sizeof(failure_),
          "CC1101 snapshot read failed: register 0x%02X",registers_[i]);
        trace_stage("CC1101 register snapshot",false);
        return false;
      }
    }
    trace_stage("CC1101 registers snapshotted",true);
    active_=true; // every later failure MUST restore RX and verify
    return true;
  }
  bool prepare_tx() override {
    failure_[0]='\0';
    if (!active_ || driver_installed_) return fail_step_("TX prepare requires acquired idle radio");
    radio_.set_idle();
    radio_.set_packet_mode(false); // detach the normal CC1101 packet IRQ consumer
    if (!bus_.strobe(0x36)) return fail_step_("CC1101 SIDLE strobe (0x36)");
    trace_stage("CC1101 entered IDLE",true);
    if (!bus_.strobe(0x3B)) return fail_step_("CC1101 SFTX strobe (0x3B)");
    trace_stage("CC1101 TX FIFO flushed",true);

    // ESP-IDF v5.5: install BEFORE uart_param_config. Installing the driver
    // resets peripheral UART registers; a prior 8N1/baud configuration may
    // otherwise silently be discarded. Keep the driver_installed_ flag set
    // as soon as install succeeds so failed later stages still tear it down.
    if (!check_esp_("UART1 driver install",
        uart_driver_install(UART_NUM_1,512,1024,0,nullptr,0))) return false;
    driver_installed_=true;
    uart_config_t cfg{};
    cfg.baud_rate=static_cast<int>(preset_.baud);
    cfg.data_bits=UART_DATA_8_BITS;
    cfg.parity=UART_PARITY_DISABLE;
    cfg.stop_bits=UART_STOP_BITS_1;
    cfg.flow_ctrl=UART_HW_FLOWCTRL_DISABLE;
    cfg.source_clk=UART_SCLK_DEFAULT;
    if (!check_esp_("UART1 8N1/baud config",
        uart_param_config(UART_NUM_1,&cfg)))return false;

    // High UART idle level, before the CC1101 is switched to TX.
    const auto pin=static_cast<gpio_num_t>(pin_);
    if (!check_esp_("UART TX GPIO output direction",
        gpio_set_direction(pin,GPIO_MODE_OUTPUT)))return false;
    if (!check_esp_("UART TX GPIO idle-high",gpio_set_level(pin,1)))return false;
    if (!check_esp_("UART1 TX GPIO route",
        uart_set_pin(UART_NUM_1,pin_,UART_PIN_NO_CHANGE,
                     UART_PIN_NO_CHANGE,UART_PIN_NO_CHANGE)))return false;

    // CC1101 DEVIATN (0x15) has reserved bit 3, which reads as zero.
    // Do not accept a preset that asks for unrepresentable register bits:
    // the original 0x48 caused a deterministic readback mismatch (0x40).
    if ((preset_.deviatn & 0x08U) != 0)
      return fail_step_("CC1101 DEVIATN preset uses reserved bit 3");

    // PKTCTRL0=0x30 means asynchronous GDO0 data mode. Never fall back to
    // packet/FIFO TX silently if these writes or readbacks fail.
    const uint8_t regs[]={0x00,0x02,0x08,0x10,0x11,0x12,0x15};
    const uint8_t vals[]={0x2E,0x2E,preset_.pktctrl0,preset_.mdmcfg4,
                          preset_.mdmcfg3,preset_.mdmcfg2,preset_.deviatn};
    for(size_t i=0;i<sizeof(regs);++i) {
      if (!bus_.write(regs[i],vals[i])) {
        std::snprintf(failure_,sizeof(failure_),
                      "CC1101 write failed at register 0x%02X (value 0x%02X)",
                      regs[i],vals[i]);
        trace_stage("CC1101 modem register write",false);
        return false;
      }
    }
    trace_stage("CC1101 modem registers written",true);
    for(size_t i=0;i<sizeof(regs);++i){
      uint8_t found=0;
      const bool read_ok=bus_.read(regs[i],found);
      if(!read_ok || found!=vals[i])return fail_register_(regs[i],vals[i],found,read_ok);
    }
    trace_stage("CC1101 modem registers verified",true);
    return true;
  }
  bool transmit_frame(const uint8_t *wire,size_t count) override {
    failure_[0]='\0';
    if (!active_ || !driver_installed_ || !wire || !count)
      return fail_step_("TX frame parameters/radio lease");
    if (!bus_.strobe(0x35))return fail_step_("CC1101 STX strobe (0x35)");
    bool tx_state=false;
    uint8_t last_state=0;
    for (int attempt=0;attempt<5;++attempt) {
      uint8_t state=0;
      if (!bus_.read(0x35,state))return fail_step_("CC1101 MARCSTATE read");
      last_state=static_cast<uint8_t>(state&0x1F);
      if (last_state==0x13) {tx_state=true;break;}
      delay(1);
    }
    if (!tx_state) {
      std::snprintf(failure_,sizeof(failure_),
                    "CC1101 did not enter TX: MARCSTATE 0x%02X, expected 0x13",last_state);
      trace_stage("CC1101 entered TX",false);
      return false;
    }
    trace_stage("CC1101 entered TX",true);
    const int written=uart_write_bytes(UART_NUM_1,wire,count);
    if (written!=static_cast<int>(count)) {
      std::snprintf(failure_,sizeof(failure_),
                    "UART1 wrote %d of %u bytes",written,static_cast<unsigned>(count));
      trace_stage("UART1 transmit enqueue",false);
      return false;
    }
    trace_stage("UART1 transmit enqueue",true);
    const uint32_t ms=static_cast<uint32_t>((count*10000ULL+preset_.baud-1)/preset_.baud)+250U;
    if (!check_esp_("UART1 TX drain",
        uart_wait_tx_done(UART_NUM_1,pdMS_TO_TICKS(ms))))return false;
    delay(1); // drain the final stop bit before SIDLE
    if(!bus_.strobe(0x36))return fail_step_("CC1101 SIDLE after TX");
    trace_stage("CC1101 left TX",true);
    return true;
  }
  void between_frames(uint16_t gap) override { if(gap)delay(gap); }
  bool restore_rx() override {
    if(!active_)return fail_step_("RX restore without TX radio lease");
    bool ok=true;
    // No automatic retry: a failed RX restoration must stay fail-closed.
    if(!bus_.strobe(0x36)) { fail_step_("CC1101 SIDLE for RX restore"); ok=false; }
    if(driver_installed_) {
      const esp_err_t result=uart_driver_delete(UART_NUM_1);
      if(result!=ESP_OK) {fail_esp_("UART1 driver delete",result);ok=false;}
      else {driver_installed_=false;trace_stage("UART1 driver removed",true);}
    }
    const esp_err_t gpio_result=gpio_reset_pin(static_cast<gpio_num_t>(pin_));
    if(gpio_result!=ESP_OK){fail_esp_("UART TX GPIO release",gpio_result);ok=false;}
    // Identical recovery sequence for TX and temporary RAMSES RX leasing.
    if(!restore_cc1101_packet_rx(radio_)) {
      fail_step_("CC1101 RX profile / packet IRQ rearm");
      ok=false;
    } else {
      trace_stage("ESPHome packet IRQ rearmed after UART release",true);
      trace_stage("CC1101 restored packet RX",true);
    }
    return ok;
  }
  bool verify_rx() override {
    if(!active_ || radio_.is_failed() || driver_installed_)
      return fail_step_("RX verification preconditions");
    for(size_t i=0;i<registers_.size();++i) {
      uint8_t current=0;
      const bool read_ok=bus_.read(registers_[i],current);
      if(!read_ok || current!=snapshot_[i])
        return fail_register_(registers_[i],snapshot_[i],current,read_ok);
    }
    uint8_t state=0;
    if(!bus_.read(0x35,state))return fail_step_("CC1101 RX MARCSTATE read");
    if((state&0x1F)!=0x0D){
      std::snprintf(failure_,sizeof(failure_),
        "CC1101 RX MARCSTATE 0x%02X, expected 0x0D",state&0x1F);
      trace_stage("CC1101 RX state verification",false);
      return false;
    }
    trace_stage("CC1101 RX registers/state verified",true);
    active_=false;
    return true;
  }
  cc1101::CC1101Component &radio_;
  ramses_spi::CC1101Bus bus_;
  RadioModeManager &mode_;
  uint8_t channel_;
  int pin_;
  GdoUartPreset preset_;
  bool enabled_{false};
  bool active_{false};
  bool driver_installed_{false};
  static constexpr std::array<uint8_t,14> registers_{{0x00,0x02,0x03,0x04,0x05,0x06,
                                   0x07,0x08,0x10,0x11,0x12,0x15,0x17,0x18}};
  std::array<uint8_t,14> snapshot_{};
  GdoSerialTxSession session_;
  char failure_[192]{};
  TraceObserver observer_{nullptr};
  void *context_{nullptr};
};
}} // namespace
#endif
