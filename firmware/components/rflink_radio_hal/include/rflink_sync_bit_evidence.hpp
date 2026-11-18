#pragma once
// User-armed, bounded, immutable 868-MHz synchronous bit evidence.
//
// A fixed-size block array is caller-owned (PSRAM on ESP32-S3). This class is
// platform-independent and only called from the serialized RX worker/task;
// network reads are synchronized by the owning receiver. No raw RF data is
// logged or exposed by the ordinary production firmware.
//
// Captured blocks are NOT decoded packets. Bit offsets, discontinuities and
// acquisition epochs must be preserved for offline replay.
#include <cstddef>
#include <cstdint>
#include "rflink_clocked_bit_capture.hpp"

namespace rflink {

struct SyncBitEvidenceStats {
  bool ready = false;
  bool recording = false;
  bool full = false;
  uint32_t generation = 0;
  uint32_t capacity = 0;
  uint32_t used = 0;
  uint32_t discarded_newest = 0;
  uint32_t recorded_discontinuities = 0;
  uint32_t recorded_bits = 0;
};

class SyncBitEvidence {
 public:
  // Attach a preallocated, fixed-capacity contiguous block array.
  // Caller must make sure no exporter/producer is active; do not own memory.
  bool attach(ClockedBitBlock* blocks, size_t capacity) {
    if (!blocks || capacity == 0 || capacity > UINT32_MAX) return false;
    if (stats_.recording) return false;
    blocks_ = blocks;
    stats_ = SyncBitEvidenceStats{};
    stats_.ready = true;
    stats_.capacity = static_cast<uint32_t>(capacity);
    return true;
  }
  // Explicit user action clears retained evidence. Never auto-start at boot.
  bool arm() {
    if (!stats_.ready) return false;
    stats_.recording = true;
    stats_.full = false;
    stats_.used = 0;
    stats_.discarded_newest = 0;
    stats_.recorded_discontinuities = 0;
    stats_.recorded_bits = 0;
    if (++stats_.generation == 0) ++stats_.generation;
    return true;
  }
  bool freeze() {
    if (!stats_.ready || !stats_.recording) return false;
    stats_.recording = false;
    return true;
  }
  // Only immutable, completed block objects from ClockedBitCapture may enter.
  // Drop newest and stop recording once full. Never overwrite earlier blocks.
  bool append(const ClockedBitBlock& block) {
    if (!stats_.recording) return false;
    if (block.bit_count == 0 || block.bit_count > ClockedBitBlock::kBitCapacity ||
        block.epoch == 0) return false;
    if (stats_.used >= stats_.capacity) {
      ++stats_.discarded_newest;
      stats_.recording = false;
      stats_.full = true;
      return false;
    }
    blocks_[stats_.used++] = block;
    stats_.recorded_bits += block.bit_count;
    if (block.discontinuity_before) ++stats_.recorded_discontinuities;
    // A fixed budget is a deliberate capture boundary, not evidence loss;
    // this state is distinguishable from hardware/sample-buffer overflow.
    if (stats_.used == stats_.capacity) {
      stats_.recording = false;
      stats_.full = true;
    }
    return true;
  }
  // Copy only a frozen snapshot, never return a mutable pointer to PSRAM.
  bool copy(uint32_t index, ClockedBitBlock* out) const {
    if (!out || !stats_.ready || stats_.recording || index >= stats_.used)
      return false;
    *out = blocks_[index];
    return true;
  }
  SyncBitEvidenceStats stats() const { return stats_; }

 private:
  ClockedBitBlock* blocks_ = nullptr;
  SyncBitEvidenceStats stats_{};
};

}  // namespace rflink
