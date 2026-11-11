#pragma once
// UART receive is routed from the 868MHz CC1101 GDO2 only. SPI remains the
// only CC1101 register bus (shared with two other radios, CSn still distinct).
// LAB ONLY: no Orcon TX or pairing is exposed by this backend.
#ifdef USE_ESP32
#include <algorithm>
#include "radio_mode_manager.h"
#include "ramses_spi_esp_backend.h"  // CC1101Bus uses the existing ESPHome SPI owner
#include "ramses_uart_stream.h"
#include "cc1101_packet_rx_restore.h"
#include "driver/gpio.h"
#include "driver/uart.h"

namespace esphome { namespace rflink_universal {
class CC1101UartBackend : public RadioModeManager::Backend {
 public:
  explicit CC1101UartBackend(cc1101::CC1101Component &radio, int gdo2_pin)
       : radio_(radio), bus_(radio), pin_(gdo2_pin) {}
  bool pause_itho() override {
    if(paused_ || radio_.is_failed() || pin_<0 || pin_>48)return false;
    for(size_t i=0;i<kCount;++i)if(!bus_.read(regs_[i],before_[i]))return false;
    paused_=true;  // must restore on any subsequent partial failure
    radio_.set_idle();
    radio_.set_packet_mode(false);  // detach packet FIFO/IRQ consumer
    return true;
  }
  bool start_ramses_rx() override {
    if(!paused_ || started_ || !bus_.strobe(0x36))return false;
    if(uart_is_driver_installed(UART_NUM_1))return false; // never steal UART from another component
    uart_config_t cfg{};
    cfg.baud_rate=38400;
    cfg.data_bits=UART_DATA_8_BITS;
    cfg.parity=UART_PARITY_DISABLE;
    cfg.stop_bits=UART_STOP_BITS_1;
    cfg.flow_ctrl=UART_HW_FLOWCTRL_DISABLE;
    cfg.source_clk=UART_SCLK_DEFAULT;
    if(uart_param_config(UART_NUM_1,&cfg)!=ESP_OK)return false;
    if(uart_set_pin(UART_NUM_1,UART_PIN_NO_CHANGE,pin_,UART_PIN_NO_CHANGE,UART_PIN_NO_CHANGE)!=ESP_OK)return false;
    if(uart_driver_install(UART_NUM_1,1024,0,0,nullptr,0)!=ESP_OK)return false;
    driver_installed_=true;
    if(!bus_.strobe(0x3A) || !bus_.strobe(0x3B) ||
       !bus_.write(0x00,0x0D) ||   // IOCFG2: asynchronous demodulated data
       !bus_.write(0x02,0x2E) ||   // IOCFG0: high impedance (TX not active)
       !bus_.write(0x08,0x30) ||   // PKTCTRL0: asynchronous serial mode
       !bus_.write(0x12,0x00) ||   // MDMCFG2: 2-FSK, no HW sync
       !bus_.write(0x10,0x5A) ||   // 38.4kbit/s, bandwidth profile
       !bus_.write(0x11,0x83) ||
       !bus_.write(0x15,0x40))return false;
    const uint8_t reg_ids[]={0x00,0x08,0x12,0x10,0x11,0x15};
    const uint8_t values[]={0x0D,0x30,0x00,0x5A,0x83,0x40};
    for(size_t i=0;i<6;++i) {uint8_t got=0;if(!bus_.read(reg_ids[i],got)||got!=values[i])return false;}
    if(!bus_.strobe(0x34))return false;
    uart_flush_input(UART_NUM_1);
    framer_.reset();
    started_=true;
    return true;
  }
  bool stop_ramses() override {
    bool ok=true;
    if(paused_)ok=bus_.strobe(0x36);
    if(driver_installed_) {if(uart_driver_delete(UART_NUM_1)!=ESP_OK)ok=false;else driver_installed_=false;}
    started_=false;framer_.reset();
    // UART RX on GDO2 has replaced the original GPIO matrix routing.
    // A UART driver delete alone does not restore the original pin mode.
    if (pin_>=0 && pin_<=48 && gpio_reset_pin(static_cast<gpio_num_t>(pin_))!=ESP_OK) ok=false;
    return ok;
  }
  bool restore_itho() override {
    if(!paused_)return !radio_.is_failed();
    if(driver_installed_ && !stop_ramses())return false;
    // Full packet modem + GDO0 IRQ restoration, same sequence as TX.
    return restore_cc1101_packet_rx(radio_);
  }
  bool verify_itho() override {
    if(!paused_ || radio_.is_failed() || driver_installed_)return false;
    for(size_t i=0;i<kCount;++i) {
      uint8_t got=0;
      if(!bus_.read(regs_[i],got) || got!=before_[i])return false;
    }
    uint8_t marc=0;
    if(!bus_.read(0x35,marc) || (marc&0x1F)!=0x0D)return false;
    paused_=false;
    return true;
  }
  bool poll(uint32_t now_ms, rflink_ramses_ii::Frame &frame) {
    if(!started_ || !driver_installed_)return false;
    if(framer_.take_pending(frame)) {++stats_.rx_frames;return true;}
    uint8_t bytes[96];
    const int n=uart_read_bytes(UART_NUM_1, bytes, sizeof(bytes), 0);
    if(n<0){failed_=true;return false;}
    if(!n)return false;
    ++stats_.rx_polls;
    stats_.rx_bytes+=uint32_t(n);
    const bool matched=framer_.feed(bytes,size_t(n),now_ms,frame);
    stats_.rx_invalid=framer_.rejected();
    stats_.rx_overflows=framer_.overflow();
    if(matched)++stats_.rx_frames;
    return matched;
  }
  bool error() const {return failed_;}
  const ramses_spi::Stats &stats() const {return stats_;}
 private:
  static constexpr size_t kCount=14;
  static constexpr uint8_t regs_[kCount]={0x00,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x10,0x11,0x12,0x15,0x17,0x18};
  cc1101::CC1101Component &radio_;
  ramses_spi::CC1101Bus bus_;
  RamsesUartStream framer_{};
  ramses_spi::Stats stats_{};
  int pin_{-1};
  std::array<uint8_t,kCount> before_{};
  bool paused_{false},driver_installed_{false},started_{false},failed_{false};
};
}}
#endif
