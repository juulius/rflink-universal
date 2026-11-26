#pragma once
// Passive, sampled carrier activity. No packet/framing or decoder gating.
#include <cstdint>
namespace rflink {
enum class RxActivityEnd : uint8_t { low=0, gap=1, stopped=2 };
struct RxActivityPeriod {
  uint64_t first_high_us=0,last_high_us=0;
  uint32_t id=0,high_samples=0,missed_samples=0;
  int16_t min_rssi_x2=0,max_rssi_x2=0;
  bool partial_start=false;
  RxActivityEnd end=RxActivityEnd::low;
};
struct RxActivityStats {
  uint32_t bursts=0, partial_starts=0, samples=0, carrier_samples=0;
  uint32_t missed_samples=0, gap_resets=0;
  uint64_t max_gap_us=0, last_sample_us=0;
  uint32_t max_read_us=0;
  uint64_t read_time_us=0,valid_gap_sum_us=0;
  uint32_t valid_gap_count=0,completed_periods=0;
  int16_t rssi_dbm_x2=0, min_rssi_dbm_x2=0, max_rssi_dbm_x2=0;
  bool valid=false, carrier=false, active=false;
};
class RxActivityTracker {
 public:
  static constexpr uint64_t kSampleIntervalUs=5000;
  using PeriodObserver=void(*)(void*,const RxActivityPeriod&);
  void set_period_observer(PeriodObserver observer,void* context){
    observer_=observer;context_=context;
  }
  static constexpr uint64_t kMaxContinuousGapUs=30000;
  // The actual sampling interval is limited by the existing RX worker.
  // Initial high samples or highs after gaps are observed segments,
  // not proof of an RF onset. Two low samples end a segment.
  static constexpr uint8_t kEndLowSamples=2;
  static int16_t approximate_rssi_x2(uint8_t raw){
    return int16_t(raw<128?int(raw):int(raw)-256)-148;
  }
  void reset_window(RxActivityEnd reason=RxActivityEnd::stopped){
    finish(reason);
    have_sample_=false;low_samples_=0;stats_.valid=false;
    stats_.carrier=false;stats_.active=false;stats_.last_sample_us=0;
  }
  void missed(uint32_t read_us){
    ++stats_.missed_samples;stats_.valid=false;
    stats_.read_time_us+=read_us;
    if(stats_.active && period_.missed_samples<UINT32_MAX)++period_.missed_samples;
    if(read_us>stats_.max_read_us)stats_.max_read_us=read_us;
  }
  void observe(uint64_t now_us,bool carrier,uint8_t rssi,uint32_t read_us){
    stats_.read_time_us+=read_us;
    if(read_us>stats_.max_read_us)stats_.max_read_us=read_us;
    if(have_sample_){
      const uint64_t gap=now_us-stats_.last_sample_us;
      if(gap>stats_.max_gap_us)stats_.max_gap_us=gap;
      stats_.valid_gap_sum_us+=gap;++stats_.valid_gap_count;
      if(gap>kMaxContinuousGapUs){++stats_.gap_resets;reset_window(RxActivityEnd::gap);}
    }
    const bool partial=!have_sample_;
    stats_.last_sample_us=now_us;have_sample_=true;stats_.valid=true;
    stats_.carrier=carrier;
    const auto dbm=approximate_rssi_x2(rssi);
    if(!stats_.samples || dbm<stats_.min_rssi_dbm_x2)stats_.min_rssi_dbm_x2=dbm;
    if(!stats_.samples || dbm>stats_.max_rssi_dbm_x2)stats_.max_rssi_dbm_x2=dbm;
    stats_.rssi_dbm_x2=dbm;
    ++stats_.samples;
    if(carrier){
      ++stats_.carrier_samples;low_samples_=0;
      if(!stats_.active){
        ++stats_.bursts;if(partial)++stats_.partial_starts;
        stats_.active=true;
        period_={};period_.id=stats_.bursts;period_.partial_start=partial;
        period_.first_high_us=now_us;period_.min_rssi_x2=period_.max_rssi_x2=dbm;
      }
      period_.last_high_us=now_us;
      if(period_.high_samples<UINT32_MAX)++period_.high_samples;
      if(dbm<period_.min_rssi_x2)period_.min_rssi_x2=dbm;
      if(dbm>period_.max_rssi_x2)period_.max_rssi_x2=dbm;
    }else if(stats_.active && ++low_samples_>=kEndLowSamples){
      finish(RxActivityEnd::low);
      stats_.active=false;low_samples_=0;
    }
  }
  RxActivityStats stats() const{return stats_;}
 private:
  void finish(RxActivityEnd reason){
    if(!stats_.active)return;
    period_.end=reason;++stats_.completed_periods;
    if(observer_)observer_(context_,period_);
    stats_.active=false;
  }
  PeriodObserver observer_=nullptr;
  void* context_=nullptr;
  RxActivityPeriod period_{};
  RxActivityStats stats_{};
  bool have_sample_=false;
  uint8_t low_samples_=0;
};
} // namespace rflink
