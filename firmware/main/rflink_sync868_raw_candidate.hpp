#pragma once
// Experimental protocol-independent preamble evidence sampler.
// A run of alternating bits is only a candidate trigger, not a frame,
// a carrier measurement, or a proof of a message. Retain a fixed exact
// 504-bit window to fit the existing 63-byte isolated detail arena.
// No SPI, allocations, GPIO, ISR or radio mode changes.
#include <array>
#include <cstdint>
#include "rflink_clocked_bit_capture.hpp"

namespace rflink {
struct Sync868RawCandidate {
  static constexpr uint16_t kBits=504;
  std::array<uint8_t,kBits/8> bytes{};
  uint32_t epoch=0;
};
class Sync868RawCandidateDetector {
 public:
  static constexpr unsigned kPreambleBits=32;
  // Bound writes even if a strong interferer mimics an alternating preamble.
  static constexpr unsigned kHoldoffBits=8192;
  using Observer=void(*)(void*,const Sync868RawCandidate&);
  void consume(const ClockedBitBlock& block,uint32_t sequence,
               Observer observer,void* context) {
    if(!have_block_ || sequence!=last_sequence_+1u ||
       block.epoch!=epoch_ || block.discontinuity_before || previous_end_)
      reset();
    if(block.bit_count>ClockedBitBlock::kBitCapacity || !block.epoch) {
      reset();have_block_=false;return;
    }
    have_block_=true;last_sequence_=sequence;epoch_=block.epoch;
    previous_end_=block.window_end;
    for(unsigned i=0;i<block.bit_count;++i) {
      const bool bit=(block.bytes[i/8] & (0x80u>>(i%8)))!=0;
      if(remaining_holdoff_)--remaining_holdoff_;
      alternating_=have_bit_ && bit!=last_bit_
        ? (alternating_<kPreambleBits?alternating_+1:kPreambleBits):1;
      have_bit_=true;last_bit_=bit;
      if(alternating_<kPreambleBits)latched_=false;
      if(used_) {
        append(bit);
        if(used_==Sync868RawCandidate::kBits) {
          if(observer)observer(context,candidate_);
          used_=0;remaining_holdoff_=kHoldoffBits;
        }
      } else if(alternating_==kPreambleBits && !latched_) {
        latched_=true;
        if(remaining_holdoff_)continue;
        candidate_={};candidate_.epoch=epoch_;
        // The previous 32 samples are known exactly by the alternating run.
        for(unsigned j=0;j<kPreambleBits;++j)
          append(bit ^ bool((kPreambleBits-1-j)&1u));
      }
    }
    if(block.window_end)reset();
  }
 private:
  void append(bool bit) {
    if(bit)candidate_.bytes[used_/8]|=uint8_t(0x80u>>(used_%8));
    ++used_;
  }
  void reset() {
    used_=0;alternating_=0;have_bit_=false;latched_=false;
    // Keep holdoff across resets to limit log traffic.
  }
  Sync868RawCandidate candidate_{};
  uint32_t epoch_=0,last_sequence_=0;
  unsigned used_=0,alternating_=0,remaining_holdoff_=0;
  bool have_block_=false,previous_end_=false,have_bit_=false,last_bit_=false,latched_=false;
};
}
