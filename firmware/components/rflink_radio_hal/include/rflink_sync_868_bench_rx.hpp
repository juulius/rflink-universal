#pragma once
// Manager-owned experimental fixed synchronous 868-MHz receive transport.
//
// This class owns one CC1101 sync profile and a bounded GDO DATA+CLOCK sampler.
// It is NOT started in production builds; only an explicitly enabled
// bench-only main.cpp build creates it. No automatic FIFO/UART fallback.
// Real RF compatibility, data/clock polarity and ISR load remain unproven.
#include <cstdint>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "rflink_cc1101_spi.hpp"
#include "rflink_clocked_bit_capture.hpp"
#include "rflink_sync_gdo_sampler.hpp"

namespace rflink {

struct Sync868BenchStats {
  bool listening = false;
  uint32_t epoch = 0;
  uint32_t start_failures = 0;
  uint32_t complete_blocks = 0; // Completed bit blocks are not RF frames.
  uint32_t blocks_with_gap = 0;
  uint32_t last_bit_count = 0;
  uint64_t last_bit_time_us = 0;
  SyncGdoSamplerStats sampler{};
  ClockedBitStats capture{};
};

class Cc1101Sync868BenchRx final {
 public:
  explicit Cc1101Sync868BenchRx(Cc1101SpiBus& bus)
      : bus_(bus),sampler_(capture_) {}
  ~Cc1101Sync868BenchRx();
  Cc1101Sync868BenchRx(const Cc1101Sync868BenchRx&)=delete;
  Cc1101Sync868BenchRx& operator=(const Cc1101Sync868BenchRx&)=delete;

  // Caller MUST own RadioManager's RX lease for radio 2 and have stopped
  // the prior transport. This configures the SAME fixed modem on every start.
  esp_err_t start(RadioId radio,const RfProfile& profile);
  // Stop ISR first, then put the CC1101 into IDLE. No FIFO replacement.
  esp_err_t stop(RadioId radio);
  // Worker context: nonblocking serialized drain; never called from ISR.
  void poll_once();
  Sync868BenchStats stats() const;

 private:
  bool make_mutex();
  esp_err_t install_profile(const RfProfile& profile);
  esp_err_t stop_locked();
  void collect_completed();

  Cc1101SpiBus& bus_;
  ClockedBitCapture capture_{};
  Cc1101SyncGdoSampler sampler_;
  SemaphoreHandle_t guard_ = nullptr;
  bool listening_ = false;
  uint32_t epoch_ = 0;
  uint32_t start_failures_ = 0;
  uint32_t complete_blocks_ = 0;
  uint32_t blocks_with_gap_ = 0;
  uint32_t last_bit_count_ = 0;
  uint64_t last_bit_time_us_ = 0;
};

} // namespace rflink
