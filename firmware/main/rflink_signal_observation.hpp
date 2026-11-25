#pragma once
// Observational features of retained RF evidence, not a protocol decoder.
// Called on immutable snapshots in the HTTP/CSV path, never the RX worker.
// Fixed-size scratch; no allocations or changes to retention/acceptance.
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace rflink {
struct PulseObservation {
  uint64_t total_us=0;
  uint32_t min_us=0,max_us=0;
  uint16_t high=0,low=0,zero=0,short_durations=0,level_changes=0;
  uint16_t timing_samples=0,outside_bins=0,top_two_support=0;
  uint16_t high_modes=0,low_modes=0; // Counts of occupied width bins, not bits.
  const char* pattern() const {
    if(timing_samples<8 || high<2 || low<2)return "insufficient_timing";
    return uint32_t(top_two_support)*4u>=uint32_t(timing_samples)*3u
      ?"concentrated_timing":"varied_timing";
  }
};
// Compare at most 64 rounded 50-us width bins per polarity.
// Durations shorter than 30 us or zero are reported separately; longer
// out-of-range widths remain in the timing denominator, not clamped.
inline PulseObservation observe_pulses(const uint8_t* bytes,size_t count){
  PulseObservation o{};
  if(!bytes || count>1024)return o;
  std::array<std::array<uint16_t,64>,2> histogram{};
  int previous_level=0;
  for(size_t i=0;i<count;++i){
    int32_t duration=0;
    std::memcpy(&duration,bytes+i*sizeof(duration),sizeof(duration));
    if(!duration){++o.zero;previous_level=0;continue;}
    const int level=duration>0?1:-1;
    const uint32_t width=static_cast<uint32_t>(duration>0?int64_t(duration):-int64_t(duration));
    if(previous_level && level!=previous_level)++o.level_changes;
    previous_level=level;
    if(level>0)++o.high;else ++o.low;
    o.total_us+=width;
    if(!o.min_us || width<o.min_us)o.min_us=width;
    if(width>o.max_us)o.max_us=width;
    if(width<30){++o.short_durations;continue;}
    ++o.timing_samples;
    const uint32_t bin=(width+25u)/50u;
    if(bin>=64){++o.outside_bins;continue;}
    ++histogram[level>0?0:1][bin];
  }
  for(size_t polarity=0;polarity<2;++polarity){
    uint16_t first=0,second=0,occupied=0;
    for(const auto n:histogram[polarity]){
      if(n)++occupied;
      if(n>=first){second=first;first=n;}
      else if(n>second)second=n;
    }
    o.top_two_support+=first+second;
    if(polarity==0)o.high_modes=occupied;else o.low_modes=occupied;
  }
  return o;
}

struct BitObservation {
  uint16_t bits=0,ones=0,transitions=0,alternating_prefix=0,longest_run=0;
  uint16_t tail_bits=0,tail_ones=0,tail_transitions=0,tail_longest_run=0;
  const char* pattern() const {
    if(tail_bits<8)return "insufficient_tail";
    if(uint32_t(tail_longest_run)*2u>=tail_bits)return "long_constant_run";
    if(uint32_t(tail_transitions)*10u>=uint32_t(tail_bits-1u)*9u)
      return "predominantly_alternating";
    return "mixed_bits";
  }
};
// The first 32 alternating bits triggered the existing capture. Describe
// the rest separately to avoid treating that selection bias as quality.
inline BitObservation observe_clocked_bits(const uint8_t* bytes,size_t count){
  BitObservation o{};
  if(!bytes || count==0 || count>63)return o;
  o.bits=static_cast<uint16_t>(count*8u);
  bool previous=false;
  uint16_t run=0,tail_run=0;
  for(uint16_t i=0;i<o.bits;++i){
    const bool bit=(bytes[i/8]&(0x80u>>(i%8)))!=0;
    if(bit)++o.ones;
    if(i && bit!=previous)++o.transitions;
    if(i==0 || bit!=previous)run=1;else ++run;
    if(run>o.longest_run)o.longest_run=run;
    if(i==0 || (o.alternating_prefix==i && bit!=previous))
      ++o.alternating_prefix;
    if(i>=32){
      ++o.tail_bits;
      if(bit)++o.tail_ones;
      if(i>32 && bit!=previous)++o.tail_transitions;
      if(i==32 || bit!=previous)tail_run=1;else ++tail_run;
      if(tail_run>o.tail_longest_run)o.tail_longest_run=tail_run;
    }
    previous=bit;
  }
  return o;
}
} // namespace rflink
