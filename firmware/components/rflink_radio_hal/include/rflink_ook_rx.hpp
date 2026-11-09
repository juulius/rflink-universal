#pragma once
#include "rflink_cc1101_spi.hpp"
#include "rflink_rmt_rx.hpp"
#include "freertos/semphr.h"
#include <cstdint>
#include <mutex>
namespace rflink {
// Source of a failed 433 MHz start BEFORE the RMT RX driver is even called.
// Stable stage IDs for local status/Builder diagnosis; 7 in the manager
// merely means "driver_failure" and does not identify this failing step.
enum class OokStartStage : uint8_t {
  none=0, lock=1, profile=2, hardware=3, idle=4, frequency=5,
  modem_write=6, modem_read=7, modem_verify=8, rmt=9, enter_rx=10
};
struct OokStartDiagnostics {
  OokStartStage stage=OokStartStage::none;
  int32_t esp_error=0;
  int32_t register_address=-1; // -1 means failure was not register-specific
  int32_t expected=-1;
  int32_t observed=-1;
  uint32_t attempts=0;
  uint32_t failures=0;
  bool listening=false;
};
// Uses one CC1101 OOK modem profile and RMT GDO2 frontend. The profile is
// provisional: visible as BENCH in status until checked on real hardware.
class OokPulseReceiver {
public:
  OokPulseReceiver(Cc1101SpiBus& bus, RmtPulseRx& capture, RadioId radio)
    : bus_(bus), capture_(capture), radio_(radio) {}
  esp_err_t start(const RfProfile& profile);
  esp_err_t stop();
  void poll_once();
  PulseStats stats() const { return capture_.stats(); }
  OokStartDiagnostics start_diagnostics() const {
    std::lock_guard<std::mutex> guard(diag_mutex_);
    return diagnostics_;
  }
private:
  Cc1101SpiBus& bus_;
  RmtPulseRx& capture_;
  RadioId radio_;
  SemaphoreHandle_t operation_lock_ = nullptr;
  mutable std::mutex diag_mutex_;
  OokStartDiagnostics diagnostics_{};
};
} // namespace rflink
