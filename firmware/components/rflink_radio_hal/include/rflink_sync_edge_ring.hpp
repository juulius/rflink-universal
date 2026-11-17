#pragma once
// Bounded, single-producer/single-consumer queue for CC1101 synchronous bits.
//
// Producer: one GPIO clock ISR, called on the verified data-sampling edge.
// Consumer: one serialized task, never an ISR. No allocations, SPI, logging,
// callbacks or decoder work in the producer.
//
// All objects accessed by the interrupt must reside in *internal RAM*. Reset
// only after the clock ISR has been disabled/removed; reset is NOT concurrent.
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace rflink {

struct SyncClockEdge {
  uint64_t timestamp_us = 0;
  uint32_t lost_before = 0; // Producer detected this many dropped clock edges.
  bool data_high = false;
};

class SyncClockEdgeRing {
 public:
  static constexpr uint32_t kCapacity = 2048; // ~53 ms at 38.4 ksymbols/s.
  static_assert((kCapacity & (kCapacity - 1)) == 0,
                "SPSC capacity must be a power of two");
  static_assert(std::atomic<uint32_t>::is_always_lock_free,
                "ISR queue requires a lock-free 32-bit atomic counter");

  // No concurrency permitted during reset. Total drop count persists.
  void reset_epoch() {
    head_.store(0, std::memory_order_relaxed);
    tail_.store(0, std::memory_order_relaxed);
    lost_pending_ = 0;
  }

  // Inline into IRAM-resident GPIO ISR. Drop the NEWEST incoming sample when
  // full; never overwrite earlier data and never conceal the missing edges.
  __attribute__((always_inline)) inline bool push_isr(bool bit,
                                                       uint64_t timestamp_us) {
    const uint32_t head = head_.load(std::memory_order_relaxed);
    const uint32_t tail = tail_.load(std::memory_order_acquire);
    if (static_cast<uint32_t>(head - tail) >= kCapacity) {
      dropped_.fetch_add(1, std::memory_order_relaxed);
      if (lost_pending_ != UINT32_MAX) ++lost_pending_;
      return false;
    }
    entries_[head & (kCapacity - 1)] = {timestamp_us, lost_pending_, bit};
    lost_pending_ = 0;
    head_.store(head + 1, std::memory_order_release);
    return true;
  }

  bool pop(SyncClockEdge& out) {
    const uint32_t tail = tail_.load(std::memory_order_relaxed);
    const uint32_t head = head_.load(std::memory_order_acquire);
    if (tail == head) return false;
    out = entries_[tail & (kCapacity - 1)];
    tail_.store(tail + 1, std::memory_order_release);
    return true;
  }

  uint32_t pending() const {
    const uint32_t head = head_.load(std::memory_order_acquire);
    const uint32_t tail = tail_.load(std::memory_order_acquire);
    return head - tail;
  }
  uint32_t dropped_total() const {
    return dropped_.load(std::memory_order_relaxed);
  }

  // Call only after stopping the producer and draining the queue. If the
  // stream ended while the queue was full, there is no next edge to carry
  // lost_before; the caller must explicitly mark this discontinuity.
  uint32_t trailing_loss_after_stop() {
    const uint32_t lost = lost_pending_;
    lost_pending_ = 0;
    return lost;
  }

 private:
  std::array<SyncClockEdge, kCapacity> entries_{};
  std::atomic<uint32_t> head_{0};
  std::atomic<uint32_t> tail_{0};
  std::atomic<uint32_t> dropped_{0};
  uint32_t lost_pending_ = 0; // ISR-only except after producer shutdown.
};

} // namespace rflink
