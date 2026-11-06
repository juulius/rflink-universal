#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
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
  a=d.snapshot();assert(a.unknown_packet_inputs==1);
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
  a=d.snapshot();assert(a.unknown_packet_inputs==2);
  puts("PASS native RX decoder: synthetic RAMSES fixtures, Itho/RAMSES overlap, negative cases, both pulse radios");
}
