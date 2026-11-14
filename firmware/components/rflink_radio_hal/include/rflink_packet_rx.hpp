#pragma once
#include <cstdint>
#include "esp_err.h"
#include "rflink_cc1101_spi.hpp"
#include "rflink_rx_sink.hpp"

namespace rflink {
struct PacketRxStats {
  bool configured = false;
  bool listening = false;
  uint32_t packets = 0;
  uint32_t fifo_overflows = 0;
  uint32_t spi_errors = 0;
  uint32_t recoveries = 0;
  uint32_t idle_rearms = 0; // CC1101 unexpectedly left RX (MARCSTATE=IDLE).
  uint8_t consecutive_spi_errors = 0; // 3 failures mark listening false.

  uint8_t last_marcstate = 0xff;
  uint8_t last_packet[63] = {};
  uint8_t last_length = 0;
};

// Fault notifications originate in the existing RX worker after an error
// or a completed recovery. They must not access RF SPI or retain payloads.
enum class PacketRxFault : uint8_t {
  status_spi_error=1, idle_rearmed=2, fifo_overflow=3, fifo_read_error=4
};

// Initial receive-only diagnostic path for the 868.30 MHz Itho packet profile.
// Not a RAMSES scheduler and not a full decoder. No TX access or ISR ownership.
class PacketRx868 {
public:
  explicit PacketRx868(Cc1101SpiBus& bus) : bus_(bus) {}
  esp_err_t start();
  esp_err_t stop();
  void poll_once();
  void set_frame_sink(RxFrameSink* sink) { sink_ = sink; }
  using FaultObserver=void(*)(void*,PacketRxFault,uint8_t marcstate,
                             uint8_t rxbytes,esp_err_t outcome);
  void set_fault_observer(FaultObserver observer,void* context) {
    fault_observer_=observer;fault_context_=context;
  }
  PacketRxStats stats() const;
private:
  esp_err_t recover_fifo();
  Cc1101SpiBus& bus_;
  RxFrameSink* sink_ = nullptr;
  FaultObserver fault_observer_ = nullptr;
  void* fault_context_ = nullptr;
  void report_fault(PacketRxFault cause,uint8_t marc,uint8_t rxbytes,esp_err_t outcome) {
    if(fault_observer_)fault_observer_(fault_context_,cause,marc,rxbytes,outcome);
  }
  PacketRxStats stats_{};
  void* lock_ = nullptr;
  void* operation_lock_ = nullptr; // serializes RX polling and RX stop/restart
  uint8_t idle_polls_ = 0;
};
}
