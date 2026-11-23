#pragma once
// Experimental 868 synchronous RX: bounded, task-only *live interpretation*
// reusing the exact same native Itho/RAMSES scanner as frozen RAW replay.
// Never consume/modify CC1101 registers, the radio manager or OTA state.
#include <atomic>
#include <cstdint>
#include "rflink_sync868_replay.hpp"

namespace rflink {

struct Sync868LiveStats {
  uint32_t blocks=0;
  uint32_t bits=0;
  uint32_t itho_candidates=0;          // Structural only, not CRC verified.
  uint32_t ramses_checksum_valid=0;    // Includes nested logical messages.
  uint32_t ramses_embedded=0;
  uint32_t queue_drops=0;
  uint32_t discontinuities=0;
  uint32_t scan_errors=0;
};

using Sync868LiveEvent=void (*)(void*,const Sync868ReplayHit&);

class Sync868LiveDecoder final {
 public:
  // Process only in one lower-priority consumer task (never ISR/RX worker).
  // Sequence increases for *every* completed block, including queue losses.
  void consume(const ClockedBitBlock& block,uint32_t sequence,
               Sync868LiveEvent observer,void* context);
  void note_queue_drop(); // Atomic: producer task, never emits a log.
  Sync868LiveStats stats() const;

 private:
  void process(const ClockedBitBlock& first,const ClockedBitBlock* next,
               Sync868LiveEvent observer,void* context);
  ClockedBitBlock previous_{};
  uint32_t previous_seq_=0;
  bool has_previous_=false;
  std::atomic<uint32_t> blocks_{0},bits_{0},itho_{0},ramses_{0},
      embedded_{0},queue_drops_{0},gaps_{0},errors_{0};
};

} // namespace rflink
