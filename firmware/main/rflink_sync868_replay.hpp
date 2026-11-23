#pragma once
// Shared bounded Itho/RAMSES-II codec scanner for frozen RAW replay and
// separate low-priority Experimental live RX analysis. No duplicated decoder.
// Platform-independent scanner: no radio, SPI, HTTP, RTOS, NVS or TX access.
// A 2,048-bit block is not a packet. Never cross loss/window/epoch boundaries.
#include <cstddef>
#include <cstdint>
#include "rflink_clocked_bit_capture.hpp"

namespace rflink {

enum class Sync868ReplayProtocol : uint8_t { itho=1, ramses_ii=2 };
enum class Sync868ReplayDetail : uint8_t {
  unknown=0, speed=1, timer_minutes=2, fan_percent=3,
  humidity_percent=4, command=5
};
struct Sync868ReplayHit {
  Sync868ReplayProtocol protocol=Sync868ReplayProtocol::itho;
  Sync868ReplayDetail detail=Sync868ReplayDetail::unknown;
  uint16_t value=0;
  uint16_t segment=0;
  uint32_t epoch=0;
  uint32_t bit_offset=0;   // Relative to continuous segment, NOT block.
  uint16_t span_bits=0;
  bool inverted=false;
  bool overlapping=false;
  bool embedded_in_itho=false;
};
struct Sync868ReplayReport {
  static constexpr size_t kMaxHits=40;
  uint32_t source_generation=0;
  uint32_t blocks=0;
  uint32_t recorded_bits=0;
  uint32_t segments=0;
  uint32_t discontinuities=0;
  uint32_t itho_structure_only=0;
  uint32_t ramses_checksum_valid=0;
  uint32_t ramses_embedded_in_itho=0;
  uint32_t ramses_outside_itho=0;
  uint32_t overlapping_matches=0;
  uint32_t hit_count=0;
  bool hits_truncated=false;
  bool tried_inverted=false;
  Sync868ReplayHit hits[kMaxHits]{};
};
using Sync868ReplayRead = bool (*)(void*,uint32_t,ClockedBitBlock*);
using Sync868ReplayYield = void (*)(void*,uint32_t);

// The caller must hold the receiver's immutable export lease throughout.
// The reader copies at most 256 fixed blocks; decoding uses at most two
// adjacent blocks and emits only bounded, sanitized candidate metadata.
// No automatic RF/device command or pairing is ever performed.
bool replay_sync868_frozen(uint32_t block_count,Sync868ReplayRead reader,
                          void* reader_context,bool try_inverted,
                          Sync868ReplayReport* result,
                          Sync868ReplayYield progress=nullptr,
                          void* progress_context=nullptr);
}  // namespace rflink
