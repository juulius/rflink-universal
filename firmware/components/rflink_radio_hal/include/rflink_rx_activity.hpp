#pragma once
// Passive, sampled carrier activity. No packet/framing or decoder gating.
#include <cstdint>
namespace rflink {
struct RxActivityStats {
  uint32_t bursts=0, partial_starts=0, samples=0, carrier_samples=0;
  uint32_t missed_samples=0, gap_resets=0;
  uint64_t max_gap_us=0, last_sample_us=0;
  uint32_t max_read_us=0;
  int16_t rssi_dbm_x2=0, min_rssi_dbm_x2=0, max_rssi_dbm_x2=0;
  bool valid=false, carrier=false, active=false;
};
class RxActivityTracker {
 public:
  static constexpr uint64_t kSampleIntervalUs=10000;
  static constexpr uint64_t kMaxContinuousGapUs=30000;
  // The actual sampling interval is limited by the existing RX worker.
  // Initial high samples or highs after gaps are observed segments,
  // not proof of an RF onset. Two low samples end a segment.
  static constexpr uint8_t kEndLowSamples=2;
  static int16_t approximate_rssi_x2(uint8_t raw){
    return int16_t(raw<128?int(raw):int(raw)-256)-148;
  }
  void reset_window(){
    have_sample_=false;low_samples_=0;stats_.valid=false;
    stats_.carrier=false;stats_.active=false;stats_.last_sample_us=0;
  }
  void missed(uint32_t read_us){
    ++stats_.missed_samples;stats_.valid=false;
    if(read_us>stats_.max_read_us)stats_.max_read_us=read_us;
  }
  void observe(uint64_t now_us,bool carrier,uint8_t rssi,uint32_t read_us){
    if(read_us>stats_.max_read_us)stats_.max_read_us=read_us;
    if(have_sample_){
      const uint64_t gap=now_us-stats_.last_sample_us;
      if(gap>stats_.max_gap_us)stats_.max_gap_us=gap;
      if(gap>kMaxContinuousGapUs){++stats_.gap_resets;reset_window();}
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
      }
    }else if(stats_.active && ++low_samples_>=kEndLowSamples){
      stats_.active=false;low_samples_=0;
    }
  }
  RxActivityStats stats() const{return stats_;}
 private:
  RxActivityStats stats_{};
  bool have_sample_=false;
  uint8_t low_samples_=0;
};
} // namespace rflink
