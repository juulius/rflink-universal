#pragma once
// Bounded passive carrier-triggered evidence. Task context only.
// Whole source bytes are copied; timestamps within blocks are interpolated.
// No RF framing, GPIO changes, allocation or decoder dependency.
#include <array>
#include <cstdint>
#include "rflink_clocked_bit_capture.hpp"
namespace rflink {
struct CarrierRawWindow {
  static constexpr size_t kMaxBytes=384;
  std::array<uint8_t,kMaxBytes> bytes{};
  uint64_t first_us=0,last_us=0,trigger_us=0;
  uint32_t epoch=0,activity_id=0;
  uint16_t size=0;
  bool short_history=false,limited=false;
};
struct CarrierRawStats {
  uint32_t started=0,queued=0,aborted=0,skipped=0,queue_dropped=0;
};
class CarrierRawRecorder {
 public:
  static constexpr uint64_t kPreUs=30000,kPostUs=20000,kMaxAfterTriggerUs=50000;
  // Allow time for the next 2048-bit block (~54 ms at nominal rate).
  // A stalled clock or missing SPI samples must not pin a pending window.
  static constexpr uint64_t kPendingTimeoutUs=250000;
  using Observer=bool(*)(void*,const CarrierRawWindow&);
  void set_observer(Observer cb,void* context){observer_=cb;context_=context;}
  CarrierRawStats stats()const{return stats_;}
  void reset(){
    if(pending_)++stats_.aborted;
    pending_=closed_=false;count_=next_=0;have_block_=false;last_seen_=0;
  }
  void consume(const ClockedBitBlock& b,uint32_t seq){
    if(!b.epoch || !b.bit_count || b.bit_count>ClockedBitBlock::kBitCapacity ||
       b.bit_count%8 || b.last_sample_us<=b.first_sample_us || b.window_end){
      discontinuity();return;
    }
    if(b.discontinuity_before || (have_block_ &&
        (b.epoch!=epoch_ || seq!=last_seq_+1u || b.first_sample_us<=last_us_)))
      discontinuity();
    if(pending_ && b.epoch!=trigger_epoch_)discontinuity();
    epoch_=b.epoch;last_seq_=seq;last_us_=b.last_sample_us;have_block_=true;
    blocks_[next_]=b;next_=(next_+1)%blocks_.size();
    if(count_<blocks_.size())++count_;
    finish_if_ready();
  }
  void sample(uint32_t epoch,uint64_t now,bool valid,bool carrier,bool active,uint32_t id){
    if(pending_ && (epoch!=trigger_epoch_ ||
        (now>=trigger_ && now-trigger_>=kPendingTimeoutUs)))discontinuity();
    if(!valid)return;
    if(pending_ && !closed_){
      if(id==id_ && carrier)last_high_=now;
      if(!active || id!=id_ || now>=trigger_+kMaxAfterTriggerUs){
        closed_=true;
        limited_=last_high_+kPostUs>trigger_+kMaxAfterTriggerUs;
      }
    }
    finish_if_ready();
    if(!carrier || !id || id==last_seen_)return;
    last_seen_=id;
    if(pending_){++stats_.skipped;return;}
    pending_=true;closed_=limited_=false;trigger_=last_high_=now;
    trigger_epoch_=epoch;id_=id;++stats_.started;
  }
 private:
  void discontinuity(){
    if(pending_)++stats_.aborted;
    pending_=closed_=false;count_=next_=0;have_block_=false;
    // Do not restart the same sampled activity following a bit gap.
  }
  static uint64_t estimate(const ClockedBitBlock& b,size_t i){
    const uint64_t span=b.last_sample_us-b.first_sample_us;
    const uint64_t n=b.bit_count-1u;
    return b.first_sample_us+(span/n)*i+((span%n)*i)/n;
  }
  void finish_if_ready(){
    if(!pending_ || !closed_ || !have_block_ || epoch_!=trigger_epoch_)return;
    const uint64_t start=trigger_>kPreUs?trigger_-kPreUs:0;
    const uint64_t hard_end=trigger_+kMaxAfterTriggerUs;
    const uint64_t target=last_high_+kPostUs<hard_end?last_high_+kPostUs:hard_end;
    if(last_us_<target)return;
    CarrierRawWindow out{};
    out.epoch=epoch_;out.activity_id=id_;out.trigger_us=trigger_;out.limited=limited_;
    const size_t oldest=(next_+blocks_.size()-count_)%blocks_.size();
    out.short_history=blocks_[oldest].first_sample_us>start;
    for(size_t j=0;j<count_;++j){
      const auto& b=blocks_[(oldest+j)%blocks_.size()];
      if(b.last_sample_us<start || b.first_sample_us>target)continue;
      for(size_t i=0;i<b.bit_count/8;++i){
        const auto first=estimate(b,i*8),last=estimate(b,i*8+7);
        // Include only whole original bytes lying within the requested span.
        if(first<start || last>target)continue;
        if(out.size==out.bytes.size()){out.limited=true;break;}
        if(!out.size)out.first_us=first;
        out.last_us=last;out.bytes[out.size++]=b.bytes[i];
      }
      if(out.size==out.bytes.size())break;
    }
    pending_=closed_=false;
    if(!out.size || out.first_us>trigger_ || out.last_us<trigger_){++stats_.aborted;return;}
    if(observer_ && observer_(context_,out))++stats_.queued;
    else ++stats_.queue_dropped;
  }
  std::array<ClockedBitBlock,4> blocks_{};
  size_t next_=0,count_=0;
  uint64_t last_us_=0,trigger_=0,last_high_=0;
  uint32_t epoch_=0,last_seq_=0,trigger_epoch_=0,id_=0,last_seen_=0;
  bool have_block_=false,pending_=false,closed_=false,limited_=false;
  Observer observer_=nullptr;void* context_=nullptr;
  CarrierRawStats stats_{};
};
static_assert(sizeof(CarrierRawRecorder)<=1536,"Bounded task-only bit history");
} // namespace rflink
