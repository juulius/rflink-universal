#pragma once
// Bounded clock-edge queue: one GPIO ISR producer and one RX-task consumer.
//
// IMPORTANT: Xtensa ESP-IDF's C++ 32-bit std::atomic is not guaranteed
// lock-free. For hardware, use the native FreeRTOS portMUX critical section
// in ISR/task contexts; never call generic libatomic from an IRAM ISR.
// The host-only implementation uses std::mutex for a faithful data-race test.
//
// The ring and its lock MUST reside in internal RAM. No heap, SPI, logging,
// decoder callbacks, FreeRTOS queues or task wakes from the GPIO interrupt.
// Reset only after the GPIO interrupt producer has been stopped/removed.
#include <array>
#include <cstddef>
#include <cstdint>
#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#else
#include <mutex>
#endif

namespace rflink {

struct SyncClockEdge {
  uint64_t timestamp_us = 0;
  uint32_t lost_before = 0;
  bool data_high = false;
};

class SyncClockEdgeRing {
 public:
  static constexpr uint32_t kCapacity = 2048; // ~53 ms at 38.4 ksym/s.
  static_assert((kCapacity & (kCapacity - 1)) == 0,
                "SPSC queue depth must be a power of two");

  // Producer must already be quiescent. Keep cumulative drop count.
  void reset_epoch() {
#ifdef ESP_PLATFORM
    portENTER_CRITICAL(&mux_);
#else
    std::lock_guard<std::mutex> lock(mux_);
#endif
    head_ = 0;
    tail_ = 0;
    lost_pending_ = 0;
#ifdef ESP_PLATFORM
    portEXIT_CRITICAL(&mux_);
#endif
  }

  // Always-inline ISR operation; short protected index/sample update only.
  // When full, drop NEWEST and record loss for the next accepted edge.
  __attribute__((always_inline)) inline bool push_isr(bool bit,
                                                       uint64_t timestamp_us) {
#ifdef ESP_PLATFORM
    portENTER_CRITICAL_ISR(&mux_);
#else
    std::lock_guard<std::mutex> lock(mux_);
#endif
    const bool space = static_cast<uint32_t>(head_ - tail_) < kCapacity;
    if (!space) {
      ++dropped_;
      if (lost_pending_ != UINT32_MAX) ++lost_pending_;
    } else {
      entries_[head_ & (kCapacity - 1)] = {timestamp_us, lost_pending_, bit};
      lost_pending_ = 0;
      ++head_;
    }
#ifdef ESP_PLATFORM
    portEXIT_CRITICAL_ISR(&mux_);
#endif
    return space;
  }

  bool pop(SyncClockEdge& out) {
#ifdef ESP_PLATFORM
    portENTER_CRITICAL(&mux_);
#else
    std::lock_guard<std::mutex> lock(mux_);
#endif
    const bool available = tail_ != head_;
    if (available) {
      out = entries_[tail_ & (kCapacity - 1)];
      ++tail_;
    }
#ifdef ESP_PLATFORM
    portEXIT_CRITICAL(&mux_);
#endif
    return available;
  }

  uint32_t pending() const {
#ifdef ESP_PLATFORM
    portENTER_CRITICAL(&mux_);
#else
    std::lock_guard<std::mutex> lock(mux_);
#endif
    const uint32_t n = head_ - tail_;
#ifdef ESP_PLATFORM
    portEXIT_CRITICAL(&mux_);
#endif
    return n;
  }

  uint32_t dropped_total() const {
#ifdef ESP_PLATFORM
    portENTER_CRITICAL(&mux_);
#else
    std::lock_guard<std::mutex> lock(mux_);
#endif
    const uint32_t n = dropped_;
#ifdef ESP_PLATFORM
    portEXIT_CRITICAL(&mux_);
#endif
    return n;
  }

  // Only after removing the ISR and draining pending accepted samples.
  uint32_t trailing_loss_after_stop() {
#ifdef ESP_PLATFORM
    portENTER_CRITICAL(&mux_);
#else
    std::lock_guard<std::mutex> lock(mux_);
#endif
    const uint32_t n = lost_pending_;
    lost_pending_ = 0;
#ifdef ESP_PLATFORM
    portEXIT_CRITICAL(&mux_);
#endif
    return n;
  }

 private:
  std::array<SyncClockEdge,kCapacity> entries_{};
#ifdef ESP_PLATFORM
  mutable portMUX_TYPE mux_ = portMUX_INITIALIZER_UNLOCKED;
#else
  mutable std::mutex mux_{};
#endif
  uint32_t head_ = 0;
  uint32_t tail_ = 0;
  uint32_t dropped_ = 0;
  uint32_t lost_pending_ = 0;
};

} // namespace rflink
