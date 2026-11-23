#include "rflink_sync868_live.hpp"

namespace rflink {
namespace {
struct PairSource {
  const ClockedBitBlock* first=nullptr;
  const ClockedBitBlock* next=nullptr;
};
bool read_pair(void* raw,uint32_t index,ClockedBitBlock* out) {
  if(!raw||!out||index>1)return false;
  const auto* src=static_cast<const PairSource*>(raw);
  const auto* chosen=index==0?src->first:src->next;
  if(!chosen)return false;
  *out=*chosen;
  return true;
}
}

void Sync868LiveDecoder::note_queue_drop() {
  queue_drops_.fetch_add(1,std::memory_order_relaxed);
}

void Sync868LiveDecoder::process(const ClockedBitBlock& first,
                                 const ClockedBitBlock* next,
                                 Sync868LiveEvent observer,void* context) {
  PairSource input{&first,next};
  Sync868ReplayReport report{};
  // Polarity remains unverified on real hardware. Both hypotheses are
  // evaluated, and any inverted candidate is explicitly labelled.
  if(!replay_sync868_frozen(next?2u:1u,read_pair,&input,true,&report)){
    errors_.fetch_add(1,std::memory_order_relaxed);
    return;
  }
  // Preserve only the spans of enclosing Itho frames that carry over
  // the block boundary. The next window must not emit their embedded
  // RAMSES logical packet as a second physical transmission.
  uint32_t next_covered[2]{};
  for(uint32_t i=0;i<report.hit_count;++i) {
    const auto& hit=report.hits[i];
    if(hit.segment!=0 || hit.bit_offset>=first.bit_count)continue;
    const unsigned polarity=hit.inverted?1u:0u;
    if(hit.protocol==Sync868ReplayProtocol::itho) {
      itho_.fetch_add(1,std::memory_order_relaxed);
      const uint32_t end=hit.bit_offset+hit.span_bits;
      if(next && end>first.bit_count &&
         end-first.bit_count>next_covered[polarity])
        next_covered[polarity]=end-first.bit_count;
    } else {
      ramses_.fetch_add(1,std::memory_order_relaxed);
      if(hit.embedded_in_itho || hit.bit_offset<covered_prefix_[polarity]) {
        // A nested RAMSES logical packet is NOT a second RF transmission.
        embedded_.fetch_add(1,std::memory_order_relaxed);
        continue;
      }
    }
    if(observer)observer(context,hit);
  }
  covered_prefix_[0]=next_covered[0];
  covered_prefix_[1]=next_covered[1];
}

void Sync868LiveDecoder::consume(const ClockedBitBlock& block,
                                 uint32_t sequence,
                                 Sync868LiveEvent observer,void* context) {
  // Never bridge a missing queued block, a known sampler discontinuity,
  // epoch transition or an explicitly frozen/acquisition window.
  if(has_previous_) {
    bool contiguous=sequence==previous_seq_+1u && !previous_.window_end &&
      !block.discontinuity_before && previous_.epoch==block.epoch;
    if(!contiguous) {
      gaps_.fetch_add(1,std::memory_order_relaxed);
    }
    process(previous_,contiguous?&block:nullptr,observer,context);
    has_previous_=false;
    if(!contiguous) {
      covered_prefix_[0]=0;
      covered_prefix_[1]=0;
    }
  }
  blocks_.fetch_add(1,std::memory_order_relaxed);
  bits_.fetch_add(block.bit_count,std::memory_order_relaxed);
  if(block.discontinuity_before)
    gaps_.fetch_add(1,std::memory_order_relaxed);
  if(block.window_end){
    process(block,nullptr,observer,context);
  }else{
    previous_=block;
    previous_seq_=sequence;
    has_previous_=true;
  }
}

Sync868LiveStats Sync868LiveDecoder::stats() const {
  Sync868LiveStats s{};
  s.blocks=blocks_.load(std::memory_order_relaxed);
  s.bits=bits_.load(std::memory_order_relaxed);
  s.itho_candidates=itho_.load(std::memory_order_relaxed);
  s.ramses_checksum_valid=ramses_.load(std::memory_order_relaxed);
  s.ramses_embedded=embedded_.load(std::memory_order_relaxed);
  s.queue_drops=queue_drops_.load(std::memory_order_relaxed);
  s.discontinuities=gaps_.load(std::memory_order_relaxed);
  s.scan_errors=errors_.load(std::memory_order_relaxed);
  return s;
}
} // namespace rflink
