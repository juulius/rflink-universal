#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
#include <array>
#include "rflink_native_decoder.hpp"
#include "rflink_itho_codec.hpp"
#include "rflink_ramses_codec.hpp"
#include "fixtures/synthetic_ramses.hpp"
int main(){
  using namespace rflink;
  NativeDecoder d;
  const auto packet_a=synthetic_ramses::frame(0x31d9);
  d.on_packet(RadioId::rf86830,868299900,packet_a.data(),packet_a.size());
  auto a=d.snapshot();
  assert(a.ramses_messages==1 && a.last_protocol==RecognizedProtocol::ramses_ii);
  assert(a.last_ramses_command==0x31d9);
  const auto packet_b=synthetic_ramses::frame(0x22f1);
  d.on_packet(RadioId::rf86830,868299900,packet_b.data(),packet_b.size());
  a=d.snapshot();assert(a.ramses_messages==2 && a.itho_messages==0);
  auto invalid=packet_a;invalid[0]=0xff;
  d.on_packet(RadioId::rf86830,868299900,invalid.data(),invalid.size());
  a=d.snapshot();assert(a.unknown_packet_inputs==1 &&
    a.unknown_prefix_inputs==1 && a.unknown_decode_inputs==0);
  auto data=itho::build_speed_payload(0x16,{{0x11,0x22,0x33}},1,itho::SpeedCommand::LOW);
  auto packet=itho::encode_payload(data,0xAC);
  assert(packet.size()==63);
  itho::Decoded direct{};assert(itho::decode_packet(packet,direct) && direct.speed==2);
  d.on_packet(RadioId::rf86830,868299900,packet.data(),packet.size());
  a=d.snapshot();
  // Valid Itho commands can ALSO parse as RAMSES-II; legacy score gives RAMSES precedence.
  assert(a.overlapping_decodes>=1 && a.ramses_messages==3 && a.last_protocol==RecognizedProtocol::ramses_ii);
  int32_t pulses[]={350,-1050,1050,-350};
  d.on_pulses(RadioId::rf43342,433420000,pulses,4);
  d.on_pulses(RadioId::rf43392,433920000,pulses,4);
  a=d.snapshot();assert(a.pulse_inputs_43342==1 && a.pulse_inputs_43392==1);
  d.on_packet(RadioId::rf43392,433920000,packet.data(),packet.size());
  a=d.snapshot();assert(a.rejected_inputs==1 && a.ramses_messages==3);
  unsigned char bad[63]={0};
  d.on_packet(RadioId::rf86830,868299900,bad,sizeof(bad));
  a=d.snapshot();assert(a.unknown_packet_inputs==2 &&
    a.unknown_prefix_inputs==2 && a.unknown_decode_inputs==0);
  // A packet can have the supported preamble but still fail all known
  // protocol decoders. Distinguish that case from random fixed-FIFO noise.
  std::array<uint8_t,63> undecodable{};
  undecodable[0]=0x00;undecodable[1]=0xB3;undecodable[2]=0x2A;
  undecodable[3]=0xAB;undecodable[4]=0x2A;
  d.on_packet(RadioId::rf86830,868299900,undecodable.data(),undecodable.size());
  a=d.snapshot();
  assert(a.unknown_packet_inputs==3 &&
    a.unknown_prefix_inputs==2 && a.unknown_decode_inputs==1);
  // A 512-symbol RMT capture may contain up to 1024 pulse durations.
  // The decoder can receive that raw capture without falsely decoding it.
  std::array<int32_t,1024> long_capture{};
  for(size_t i=0;i<long_capture.size();++i)long_capture[i]=i%2?-350:350;
  d.on_pulses(RadioId::rf43392,433920000,long_capture.data(),long_capture.size());
  a=d.snapshot();assert(a.pulse_inputs_43392==2);
  std::array<int32_t,1025> invalid_capture{};
  d.on_pulses(RadioId::rf43392,433920000,invalid_capture.data(),invalid_capture.size());
  a=d.snapshot();assert(a.rejected_inputs==2);
  puts("PASS native RX decoder: 868 rejection stages, bounded 433 long pulses, synthetic protocol fixtures");
}
