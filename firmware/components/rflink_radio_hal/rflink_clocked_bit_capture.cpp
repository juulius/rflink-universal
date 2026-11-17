#include "rflink_clocked_bit_capture.hpp"
#include <cstdint>

namespace rflink {

bool ClockedBitCapture::begin_epoch(uint32_t epoch) {
  if (!epoch) return false;
  if (epoch_) {
    ++stats_.epoch_resets;
    stats_.abandoned_bits += working_.bit_count;
  }
  epoch_ = epoch;
  queue_read_ = 0;
  queue_write_ = 0;
  queue_count_ = 0;
  last_clock_us_ = 0;
  have_previous_clock_ = false;
  pending_discontinuity_ = false;
  reset_working();
  return true;
}

void ClockedBitCapture::reset_working() {
  working_ = ClockedBitBlock{};
}

void ClockedBitCapture::abandon_working() {
  stats_.abandoned_bits += working_.bit_count;
  reset_working();
}

bool ClockedBitCapture::append(bool bit, uint64_t sample_us, uint32_t epoch) {
  if (!epoch || epoch != epoch_) {
    ++stats_.rejected_samples;
    return false;
  }
  if (have_previous_clock_ && sample_us < last_clock_us_) {
    ++stats_.clock_regressions;
    ++stats_.rejected_samples;
    abandon_working();
    pending_discontinuity_ = true;
    have_previous_clock_ = false;
    return false;
  }
  if (have_previous_clock_) {
    const uint64_t elapsed = sample_us - last_clock_us_;
    if (elapsed > 100) ++stats_.gap_over_100us;
    if (elapsed > stats_.max_gap_us) {
      stats_.max_gap_us =
        elapsed > UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(elapsed);
    }
  }
  last_clock_us_ = sample_us;
  have_previous_clock_ = true;

  if (!working_.bit_count) {
    working_.epoch = epoch_;
    working_.first_sample_us = sample_us;
    working_.discontinuity_before = pending_discontinuity_;
    pending_discontinuity_ = false;
  }
  const size_t i = working_.bit_count++;
  if (bit) working_.bytes[i >> 3] |= static_cast<uint8_t>(0x80u >> (i & 7u));
  working_.last_sample_us = sample_us;
  ++stats_.sample_count;
  if (working_.bit_count == ClockedBitBlock::kBitCapacity) publish(false);
  return true;
}

void ClockedBitCapture::publish(bool window_end) {
  if (!working_.bit_count) return;
  working_.window_end = window_end;
  if (queue_count_ < kQueueDepth) {
    queue_[queue_write_] = working_;
    queue_write_ = (queue_write_ + 1) % kQueueDepth;
    ++queue_count_;
    ++stats_.published_blocks;
  } else {
    // Overflow policy: preserve both older complete blocks, drop this NEW
    // block, and explicitly mark the next accepted block as discontinuous.
    ++stats_.dropped_blocks;
    stats_.dropped_bits += working_.bit_count;
    pending_discontinuity_ = true;
  }
  reset_working();
}

void ClockedBitCapture::report_lost_edges(uint32_t count) {
  if (!count || !epoch_) return;
  stats_.reported_missing_edges += count;
  abandon_working();
  pending_discontinuity_ = true;
  have_previous_clock_ = false;
}

void ClockedBitCapture::end_window() {
  if (epoch_) publish(true);
}

bool ClockedBitCapture::pop(ClockedBitBlock& out) {
  if (!queue_count_) return false;
  out = queue_[queue_read_];
  queue_read_ = (queue_read_ + 1) % kQueueDepth;
  --queue_count_;
  return true;
}

} // namespace rflink
