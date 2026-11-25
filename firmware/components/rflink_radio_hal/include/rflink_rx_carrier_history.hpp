#pragma once
// Short history of already scheduled, non-atomic RSSI/CS read pairs.
// Owned by the sync receiver's existing task mutex; never used in the ISR.
#include <array>
#include <cstddef>
#include <cstdint>

namespace rflink {
enum class CarrierContextState : uint8_t {
  invalid_timing=0, history_unavailable=1, no_matching_samples=2, sampled=3
};
struct CaptureCarrierContext {
  uint64_t estimated_start_us=0,estimated_end_us=0;
  uint64_t first_sample_us=0,last_sample_us=0;
  uint32_t epoch=0,first_burst=0,last_burst=0;
  uint16_t samples=0,carrier_samples=0,missed_samples=0;
  int16_t min_rssi_x2=0,max_rssi_x2=0;
  CarrierContextState state=CarrierContextState::invalid_timing;
  bool history_starts_late=true;
};
static_assert(sizeof(CaptureCarrierContext)<=64,"Bounded per-window context");
inline CaptureCarrierContext carrier_context_request(uint32_t epoch,
    uint64_t start,uint64_t end,bool timing_valid){
  CaptureCarrierContext c{};
  c.epoch=epoch;c.estimated_start_us=start;c.estimated_end_us=end;
  if(timing_valid && epoch && end>=start)c.state=CarrierContextState::history_unavailable;
  return c;
}

class RxCarrierHistory {
 public:
  // At most 64 samples, about 640 ms at the fastest configured cadence.
  // Delayed consumers can miss history: never use a later/current sample.
  static constexpr size_t kCapacity=64;
  void append(uint32_t epoch,uint64_t begin,uint64_t end,bool valid,
              bool carrier,int16_t rssi_x2,uint32_t burst){
    if(!epoch || end<begin)return;
    samples_[next_]={begin,end,epoch,burst,rssi_x2,valid,carrier};
    next_=(next_+1)%kCapacity;
    if(count_<kCapacity)++count_;
  }
  CaptureCarrierContext correlate(CaptureCarrierContext c) const {
    if(c.state!=CarrierContextState::history_unavailable)return c;
    c.state=CarrierContextState::no_matching_samples;
    const size_t oldest=(next_+kCapacity-count_)%kCapacity;
    for(size_t i=0;i<count_;++i){
      const auto& s=samples_[(oldest+i)%kCapacity];
      if(s.epoch!=c.epoch)continue;
      if(s.begin<=c.estimated_start_us)c.history_starts_late=false;
      // Both reads must lie within the estimated candidate window.
      if(s.begin<c.estimated_start_us || s.end>c.estimated_end_us)continue;
      if(!s.valid){++c.missed_samples;continue;}
      if(!c.samples){
        c.first_sample_us=s.begin;
        c.min_rssi_x2=c.max_rssi_x2=s.rssi_x2;
      }
      c.last_sample_us=s.end;
      if(s.rssi_x2<c.min_rssi_x2)c.min_rssi_x2=s.rssi_x2;
      if(s.rssi_x2>c.max_rssi_x2)c.max_rssi_x2=s.rssi_x2;
      ++c.samples;
      if(s.carrier){
        if(!c.carrier_samples)c.first_burst=s.burst;
        c.last_burst=s.burst;++c.carrier_samples;
      }
    }
    if(c.samples)c.state=CarrierContextState::sampled;
    return c;
  }
 private:
  struct Sample {
    uint64_t begin=0,end=0;
    uint32_t epoch=0,burst=0;
    int16_t rssi_x2=0;
    bool valid=false,carrier=false;
  };
  std::array<Sample,kCapacity> samples_{};
  size_t next_=0,count_=0;
};
static_assert(sizeof(RxCarrierHistory)<=2304,"Bounded receiver history");
} // namespace rflink
