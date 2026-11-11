// Actual pure native RF filter behavior + RMT source-clock threshold math.
// Compiled and executed on the host. Does not claim RF bench reception.
#include "rflink_pulse_filter.hpp"
#include <cassert>
#include <cstdint>
#include <limits>
#include <cstdio>
#include <algorithm>
#include <array>

static uint32_t limit(uint32_t requested_ns,uint32_t source_hz){
  assert(source_hz>0);
  const uint64_t ceiling=(uint64_t(255)*1000000000ULL)/source_hz;
  return uint32_t(std::min<uint64_t>(requested_ns,ceiling-1));
}
int main(){
  // ESP32-S3 native v0.7 exp.9 originally requested 30,000 ns,
  // but ESP-IDF 5.5.6 reported an allowable ceiling near 3,187 ns.
  assert(limit(30000,80000000)==3186);
  assert(limit(1000,80000000)==1000);
  assert(limit(30000,40000000)==6374);
  assert(limit(30000,2000000)==30000);

  std::array<int32_t,8> noise{{300,-10,300,-220,450,-400,0,0}};
  size_t n=rflink::filter_short_pulses_inplace(noise.data(),noise.size(),30);
  assert(n==4);
  assert(noise[0]==610 && noise[1]==-220);
  assert(noise[2]==450 && noise[3]==-400);

  std::array<int32_t,6> clean{{300,-400,300,-400,300,-400}};
  n=rflink::filter_short_pulses_inplace(clean.data(),clean.size(),30);
  assert(n==6 && clean[1]==-400 && clean[5]==-400);

  std::array<int32_t,5> only_noise{{5,-4,7,-2,0}};
  n=rflink::filter_short_pulses_inplace(only_noise.data(),only_noise.size(),30);
  assert(n==0);
  assert(rflink::filter_short_pulses_inplace(nullptr,0,30)==0);
  // The synthetic idle tail is an ESPHome decoding convention, not a
  // captured on-air edge. Verify both polarities, capacity and no writes.
  std::array<int32_t,5> idle{{300,-400,300,-400,777}};
  n=rflink::append_idle_termination_inplace(idle.data(),4,idle.size(),5000,false);
  assert(n==5 && idle[4]==5000);
  n=rflink::append_idle_termination_inplace(idle.data(),4,idle.size(),5000,true);
  assert(n==5 && idle[4]==-5000);
  n=rflink::append_idle_termination_inplace(idle.data(),5,idle.size(),5000,true);
  assert(n==5 && idle[4]==-5000);
  n=rflink::append_idle_termination_inplace(idle.data(),0,idle.size(),5000,false);
  assert(n==0);
  std::puts("PASS native RMT filter clamp, 30us cleanup and bounded 5ms idle tail");
}
