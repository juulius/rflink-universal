#pragma once
// RAMSES4 lab bridge: ESPHome's existing CC1101 object remains the ONLY SPI owner.
// The adapter is armed only by the explicit 'ramses_spi_lab' option and a
// manual lab RX request. TX stays inaccessible through UI/HA until RF validated.
#ifdef USE_ESP32
#include "radio_mode_manager.h"
#include "cc1101_packet_rx_restore.h"
#include "ramses_spi_fifo.h"
#include "esphome/components/cc1101/cc1101.h"
#include <array>

namespace esphome { namespace rflink_universal { namespace ramses_spi {
class CC1101Bus : public Bus {
 public:
  explicit CC1101Bus(cc1101::CC1101Component &radio) : radio_(radio) {}
  bool read(uint8_t reg, uint8_t &value) override {
    radio_.enable();
    // Status registers (0x30..0x3d): burst+read required.
    radio_.write_byte(uint8_t((reg&0x3F) | (reg>=0x30?0xC0:0x80)));
    value=radio_.transfer_byte(0x00);
    radio_.disable();
    return true;
  }
  bool write(uint8_t reg, uint8_t value) override {
    if(reg>=0x30)return false;
    radio_.enable();radio_.write_byte(reg);radio_.write_byte(value);radio_.disable();
    return true;
  }
  bool strobe(uint8_t cmd) override {
    if(cmd<0x30 || cmd>0x3D)return false;
    radio_.enable();radio_.transfer_byte(cmd);radio_.disable();return true;
  }
  bool read_fifo(uint8_t* out,size_t count) override {
    if(!out || count>64)return false;
    radio_.enable();radio_.write_byte(0xFF); // RX FIFO, burst/read
    for(size_t i=0;i<count;++i)out[i]=radio_.transfer_byte(0);
    radio_.disable();return true;
  }
  bool write_fifo(const uint8_t *data,size_t count) override {
    if(count>64 || (count&&!data))return false;
    if(!count)return true;
    radio_.enable();radio_.write_byte(0x7F); // TX FIFO, burst/write
    for(size_t i=0;i<count;++i)radio_.write_byte(data[i]);
    radio_.disable();return true;
  }
 private:
  cc1101::CC1101Component &radio_;
};

class CC1101SpiBackend : public RadioModeManager::Backend {
 public:
  explicit CC1101SpiBackend(cc1101::CC1101Component &radio, bool unsynced_capture=false)
      : radio_(radio), bus_(radio), fifo_(bus_, unsynced_capture) {}
  bool pause_itho() override {
    if(paused_ || radio_.is_failed())return false;
    radio_.set_idle(); // packet configuration must change only in IDLE
    for(size_t i=0;i<kCount;++i) if(!bus_.read(save_addr(i),before_[i]))return false;
    // set_packet_mode(false) detaches ESPHome's GDO0 packet IRQ and makes
    // the ESPHome loop ignore FIFO, WITHOUT enabling async serial transport.
    radio_.set_packet_mode(false);
    paused_=true;
    return true;
  }
  bool start_ramses_rx() override {
    if(!paused_ || fifo_.state()!=FifoTransport::State::OFF)return false;
    return fifo_.start_rx(millis());
  }
  bool stop_ramses() override {
    if(!paused_)return true;
    return fifo_.stop();
  }
  bool restore_itho() override {
    if(!paused_) return !radio_.is_failed();
    // Rebuild ESPHome's cached original Itho packet registers and GDO0 IRQ.
    // A CC1101 is in the same loop and cannot be accessed concurrently.
    return restore_cc1101_packet_rx(radio_);
  }
  bool verify_itho() override {
    if(radio_.is_failed())return false;
    for(size_t i=0;i<kCount;++i) {
      uint8_t now=0;
      if(!bus_.read(save_addr(i),now)||now!=before_[i])return false;
    }
    uint8_t marc=0;
    if(!bus_.read(0x35,marc) || (marc&0x1F)!=0x0D)return false;
    paused_=false;
    return true;
  }
  bool poll(uint32_t now, rflink_ramses_ii::Frame &out) {
    return fifo_.poll_rx(now,out);
  }
  bool error() const {return fifo_.state()==FifoTransport::State::ERROR;}
  const Stats& stats() const {return fifo_.stats();}
 private:
  static constexpr size_t kCount=14;
  // Only static configuration registers; exclude calibration and status values.
  static uint8_t save_addr(size_t i) {
    static const uint8_t regs[kCount] = {0x00,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x10,0x11,0x12,0x15,0x17,0x18};
    return regs[i];
  }
  cc1101::CC1101Component &radio_;
  CC1101Bus bus_;
  FifoTransport fifo_;
  std::array<uint8_t,kCount> before_{};
  bool paused_{false};
};

}}} // namespace
#endif // USE_ESP32
