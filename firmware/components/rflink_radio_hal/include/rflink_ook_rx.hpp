#pragma once
#include "rflink_cc1101_spi.hpp"
#include "rflink_rmt_rx.hpp"
#include "freertos/semphr.h"
namespace rflink {
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
private:
  Cc1101SpiBus& bus_;
  RmtPulseRx& capture_;
  RadioId radio_;
  SemaphoreHandle_t operation_lock_ = nullptr;
};
} // namespace rflink
