#pragma once
// Radio-independent clocked-bit capture for the proposed fixed 868 MHz 2-FSK RX.
//
// This is NOT a GPIO ISR or CC1101 driver. A future, manager-owned hardware
// adapter must sample the DATA line on the verified active CLOCK edge and
// deliver samples on one serialized execution context. Do not call these
// functions concurrently from an ISR and a task without an explicit handoff.
//
// All chunks are evidence, not decoded protocol frames. No heap, no SPI,
// no mode switching, no UART 8N1 assumption.
#include <array>
#include <cstddef>
#include <cstdint>

namespace rflink {

struct ClockedBitBlock {
  static constexpr size_t kBitCapacity = 2048;
  static constexpr size_t kByteCapacity = kBitCapacity / 8;
  std::array<uint8_t,kByteCapacity> bytes{};
  uint16_t bit_count = 0;
  uint32_t epoch = 0;
  uint64_t first_sample_us = 0;
  uint64_t last_sample_us = 0;
  bool discontinuity_before = false;
  bool window_end = false; // Explicit source boundary; NOT a decoded frame.
};

struct ClockedBitStats {
  uint32_t sample_count = 0;
  uint32_t published_blocks = 0;
  uint32_t rejected_samples = 0;
  uint32_t reported_missing_edges = 0;
  uint32_t dropped_blocks = 0; // Full queue: drop newest block, never overwrite.
  uint32_t dropped_bits = 0;
  uint32_t abandoned_bits = 0; // Incomplete current block on loss/epoch reset.
  uint32_t clock_regressions = 0;
  uint32_t gap_over_100us = 0; // Diagnostic only; does not delimit messages.
  uint32_t max_gap_us = 0;
  uint32_t epoch_resets = 0;
};

// Single-producer / single-consumer only when both execute on the SAME
// serialized task. This structure intentionally contains no ISR lock and no
// unbounded queue, so a future real driver must supply its own safe handoff.
class ClockedBitCapture {
 public:
  static constexpr size_t kQueueDepth = 2;

  // Explicit receive lease generation. Clears pending evidence from previous
  // profiles, boots or transmit handovers. Epoch zero is never valid.
  bool begin_epoch(uint32_t epoch);

  // One bit sampled on the hardware-verified correct CLOCK edge.
  // Rejected if epoch mismatches, epoch is zero or timestamps go backwards.
  bool append(bool data_high, uint64_t sample_us, uint32_t epoch);

  // A real hardware underflow/clock loss invalidates only the active fragment.
  // Must be reported by the producer; timing out alone cannot prove a loss.
  void report_lost_edges(uint32_t count);

  // Publish a partial block on a known acquisition-window boundary.
  // A boundary is capture metadata, not a validated RF packet.
  void end_window();

  // Copies immutable evidence; does not expose internal writable storage.
  bool pop(ClockedBitBlock& out);

  const ClockedBitStats& stats() const { return stats_; }
  uint32_t active_epoch() const { return epoch_; }
  size_t pending_blocks() const { return queue_count_; }
  size_t buffered_bits() const { return working_.bit_count; }

 private:
  void publish(bool window_end);
  void abandon_working();
  void reset_working();

  ClockedBitBlock working_{};
  std::array<ClockedBitBlock,kQueueDepth> queue_{};
  ClockedBitStats stats_{};
  uint64_t last_clock_us_ = 0;
  uint32_t epoch_ = 0;
  size_t queue_read_ = 0;
  size_t queue_write_ = 0;
  size_t queue_count_ = 0;
  bool have_previous_clock_ = false;
  bool pending_discontinuity_ = false;
};

} // namespace rflink
