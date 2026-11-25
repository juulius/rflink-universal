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
  uint64_t estimated_start_us=0,estimated_end_us=0;
  bool timing_valid=false;
};
// Only block endpoints carry measured timestamps. Positions between them
// are estimates, not individually recorded edge times.
struct SyncBitPosition {
  uint64_t first=0,last=0;
  uint16_t index=0,count=0;
  bool valid() const {
    return count && last>=first && (count==1?last==first:last>first);
  }
  uint64_t estimate() const {
    if(!valid() || count<=1)return first;
    const uint64_t span=last-first;
    const uint64_t intervals=count-1u;
    return first+(span/intervals)*index+((span%intervals)*index)/intervals;
  }
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
    const SyncBitPosition position{block.first_sample_us,block.last_sample_us,0,block.bit_count};
    if(used_ && !position.valid())candidate_.timing_valid=false;
    if(alternating_ && !position.valid())prefix_timing_valid_=false;
    for(unsigned i=0;i<block.bit_count;++i) {
      const bool bit=(block.bytes[i/8] & (0x80u>>(i%8)))!=0;
      if(remaining_holdoff_)--remaining_holdoff_;
      if(!have_bit_ || bit==last_bit_){
        prefix_position_=position;prefix_position_.index=static_cast<uint16_t>(i);
        prefix_timing_valid_=position.valid();
      }
      alternating_=have_bit_ && bit!=last_bit_
        ? (alternating_<kPreambleBits?alternating_+1:kPreambleBits):1;
      have_bit_=true;last_bit_=bit;
      if(alternating_<kPreambleBits)latched_=false;
      if(used_) {
        append(bit);
        if(used_==Sync868RawCandidate::kBits) {
          auto end=position;end.index=static_cast<uint16_t>(i);
          candidate_.estimated_end_us=end.estimate();
          candidate_.timing_valid=candidate_.timing_valid && end.valid() &&
            candidate_.estimated_end_us>=candidate_.estimated_start_us;
          if(observer)observer(context,candidate_);
          used_=0;remaining_holdoff_=kHoldoffBits;
        }
      } else if(alternating_==kPreambleBits && !latched_) {
        latched_=true;
        if(remaining_holdoff_)continue;
        candidate_={};candidate_.epoch=epoch_;
        candidate_.estimated_start_us=prefix_position_.estimate();
        candidate_.timing_valid=prefix_timing_valid_ && position.valid();
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
    prefix_timing_valid_=false;prefix_position_={};
    // Keep holdoff across resets to limit log traffic.
  }
  Sync868RawCandidate candidate_{};
  SyncBitPosition prefix_position_{};
  bool prefix_timing_valid_=false;
  uint32_t epoch_=0,last_sequence_=0;
  unsigned used_=0,alternating_=0,remaining_holdoff_=0;
  bool have_block_=false,previous_end_=false,have_bit_=false,last_bit_=false,latched_=false;
};
}
