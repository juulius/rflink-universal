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
#include "rflink_rf_profiles.hpp"
#include "rflink_clocked_bit_capture.hpp"
#include "rflink_sync_gdo_sampler.hpp"
#include "rflink_sync_bit_evidence.hpp"
#include "rflink_rx_activity.hpp"
#include "rflink_rx_carrier_history.hpp"

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
  RxActivityStats activity{};
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
  // Optional task-only observer of completed bit blocks. This does not
  // classify an RF frame and must NEVER block the RX worker.
  using BlockObserver=void (*)(void*,const ClockedBitBlock&,uint32_t);
  void set_block_observer(BlockObserver observer,void* context) {
    block_observer_=observer;
    block_context_=context;
  }
  Sync868BenchStats stats() const;
  // Set once before starting RX; callback is task-only, bounded and must not reenter receiver.
  void set_activity_observer(RxActivityTracker::PeriodObserver observer,void* context){
    activity_.set_period_observer(observer,context);
  }
  // Decoder task only: immediate mutex attempt, no new SPI transaction.
  CaptureCarrierContext try_correlate_activity(CaptureCarrierContext context) const;
  // Dedicated local bench-only capture controls; never automatically armed.
  // Both calls are task-only. An active CSV export blocks re-arming.
  bool arm_evidence();
  bool freeze_evidence();
  SyncBitEvidenceStats evidence_stats() const;
  // begin/copy/end own a stable, immutable export lease; network transmission
  // never holds the driver mutex while waiting for a slow HTTP client.
  bool begin_evidence_export(SyncBitEvidenceStats* snapshot);
  bool copy_evidence(uint32_t index, ClockedBitBlock* block) const;
  void end_evidence_export();

 private:
  bool make_mutex();
  esp_err_t install_profile(const RfProfile& profile);
  esp_err_t stop_locked();
  void collect_completed();
  void sample_activity();

  Cc1101SpiBus& bus_;
  ClockedBitCapture capture_{};
  Cc1101SyncGdoSampler sampler_;
  static constexpr size_t kEvidenceBlocks = 256;
  SyncBitEvidence evidence_{};
  ClockedBitBlock* evidence_memory_ = nullptr; // PSRAM, no ISR accesses.
  bool evidence_export_active_ = false;
  SemaphoreHandle_t guard_ = nullptr;
  bool listening_ = false;
  uint32_t epoch_ = 0;
  uint32_t start_failures_ = 0;
  uint32_t complete_blocks_ = 0;
  uint32_t blocks_with_gap_ = 0;
  uint32_t last_bit_count_ = 0;
  uint64_t last_bit_time_us_ = 0;
  uint32_t emitted_block_sequence_ = 0;
  RxActivityTracker activity_{};
  RxCarrierHistory carrier_history_{};
  uint64_t last_activity_attempt_us_=0;
  BlockObserver block_observer_ = nullptr;
  void* block_context_ = nullptr;
};

} // namespace rflink
