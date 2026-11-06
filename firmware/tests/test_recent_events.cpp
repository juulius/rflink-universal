#include <cassert>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <atomic>
#include "rflink_native_decoder.hpp"
#include "fixtures/synthetic_ramses.hpp"
static uint32_t time_ms(){return 123456;}
int main(){
  rflink::NativeDecoder d;
  d.set_uptime_provider(&time_ms);
  rflink::RxRecentEvent ev[rflink::kRecentEventCapacity]{};
  assert(d.recent_events(ev,rflink::kRecentEventCapacity)==0);
  const int32_t pulses[]={350,-350,700,-700};
  const auto synthetic_packet=synthetic_ramses::frame(0x31d9);
  d.on_packet(rflink::RadioId::rf86830,868299900,synthetic_packet.data(),synthetic_packet.size());
  d.on_pulses(rflink::RadioId::rf43342,433420000,pulses,4);
  size_t n=d.recent_events(ev,rflink::kRecentEventCapacity);
  assert(n==2 && ev[0].kind==rflink::EventKind::ramses && ev[0].command==0x31d9);
  assert(ev[0].uptime_ms==123456 && ev[1].radio==rflink::RadioId::rf43342);
  assert(ev[0].sequence==1 && ev[1].sequence==2);
  // Ring wraps without exposing old or uninitialised events.
  for(unsigned i=0;i<80;++i)d.on_pulses(rflink::RadioId::rf43392,433920000,pulses,4);
  n=d.recent_events(ev,rflink::kRecentEventCapacity);
  assert(n==32 && ev[0].sequence==51 && ev[31].sequence==82);
  assert(d.recent_events(ev,4)==4 && ev[0].sequence==79 && ev[3].sequence==82);
  // API snapshot may race RX worker; event sequence and records stay consistent.
  std::atomic<bool> running{true};
  std::thread reader([&](){
    rflink::RxRecentEvent snapshot[32];
    while(running.load()){
      size_t c=d.recent_events(snapshot,32);
      for(size_t k=1;k<c;++k) assert(snapshot[k].sequence==snapshot[k-1].sequence+1);
    }
  });
  for(int i=0;i<1000;++i)d.on_pulses(rflink::RadioId::rf43342,433420000,pulses,4);
  running=false;reader.join();
  std::puts("PASS recent RF event ring: mixed radio types, bounded history, wrap, concurrent reads, no RF payloads");
}
