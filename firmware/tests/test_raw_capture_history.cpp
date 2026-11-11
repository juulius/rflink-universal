#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <memory>
#include "rflink_native_decoder.hpp"
#include "rflink_raw_capture.hpp"

static uint32_t last_observed_sequence=0;
static void on_event(void*,rflink::RadioId,rflink::EventKind,size_t,
                     uint16_t,bool,uint32_t,uint32_t sequence){
  last_observed_sequence=sequence;
}
int main(){
  using namespace rflink;
  // Mimic the embedded PSRAM lifetime: never put ~100 KiB on worker stack.
  auto history=std::make_unique<RawCaptureHistory>();
  NativeDecoder decoder;
  decoder.set_raw_capture_history(history.get());
  decoder.set_event_observer(&on_event,nullptr);
  auto sample=std::make_unique<RawCaptureEntry>();
  std::array<int32_t,1024> pulses{};
  for(size_t i=0;i<pulses.size();++i)pulses[i]=i%2?-int32_t(i+35):int32_t(i+35);
  decoder.on_pulses(RadioId::rf43392,433920000,pulses.data(),pulses.size());
  const uint32_t first=last_observed_sequence;
  assert(first==1);
  assert(decoder.raw_capture(1,first,sample.get()));
  assert(sample->kind==RawCaptureKind::pulses);
  assert(sample->count==1024 && sample->pulses_us[0]==35 &&
         sample->pulses_us[1023]==-1058);
  // Packet contents are retained even when the protocol prefix is missing.
  std::array<uint8_t,63> packet{};
  for(size_t i=0;i<packet.size();++i)packet[i]=static_cast<uint8_t>(i);
  decoder.on_packet(RadioId::rf86830,868299900,packet.data(),packet.size());
  const auto packet_seq=last_observed_sequence;
  assert(decoder.raw_capture(2,packet_seq,sample.get()));
  assert(sample->kind==RawCaptureKind::packet && sample->count==63);
  assert(sample->packet[0]==0 && sample->packet[62]==62);
  // Eight new samples in one channel evict only that channel's oldest.
  const int32_t short_frame[]={350,-800,350,-800};
  for(unsigned i=0;i<8;++i)
    decoder.on_pulses(RadioId::rf43392,433920000,short_frame,4);
  assert(!decoder.raw_capture(1,first,sample.get()));
  assert(decoder.raw_capture(2,packet_seq,sample.get()));
  assert(!decoder.raw_capture(1,0,sample.get()));
  assert(!decoder.raw_capture(3,packet_seq,sample.get()));
  assert(!decoder.raw_capture(2,999999,sample.get()));
  std::puts("PASS raw RF evidence: 433 complete pulse trains, 868 exact FIFO bytes, per-radio eviction and event correlation");
}
