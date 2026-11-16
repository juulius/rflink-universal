#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
#include <array>
#include "rflink_native_decoder.hpp"
#include "rflink_433_weather_rx.hpp"
#include "rflink_itho_codec.hpp"
#include "rflink_ramses_codec.hpp"
#include "fixtures/synthetic_ramses.hpp"
static std::vector<int32_t> synthetic_pwm(const std::vector<uint8_t>& bytes,
                                          int long_us, bool invert=false) {
  std::vector<int32_t> result;
  result.reserve(bytes.size()*16);
  for(uint8_t b:bytes)for(unsigned bit=0;bit<8;++bit){
    bool one=(b&(0x80u>>bit))!=0;
    if(invert)one=!one;
    result.push_back(one?long_us:500);
    result.push_back(-1000);
  }
  return result;
}
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

  // Native v0.6.6 checksum-protected 433.92 weather decoders. These are
  // deliberately synthetic examples, not evidence of real sensor reception.
  const std::vector<uint8_t> wh2_bytes{0xff,0x45,0xa1,0x2c,0x37,0xa5};
  auto wh2=synthetic_pwm(wh2_bytes,1500);
  auto weather=weather433::decode(wh2.data(),wh2.size(),433920000);
  assert(weather.protocol==weather433::Protocol::fineoffset_wh2 &&
         weather.integrity_verified && weather.sensor_id==0x5a &&
         weather.temperature_tenths_c==300 && weather.humidity_percent==55);
  d.on_pulses(RadioId::rf43392,433920000,wh2.data(),wh2.size());
  a=d.snapshot();
  assert(a.fineoffset_wh2_messages==1 &&
         a.last_protocol==RecognizedProtocol::fineoffset_wh2);
  RxRecentEvent events[32]{};
  size_t used=d.recent_events(events,32);
  assert(used>0 && events[used-1].kind==EventKind::fineoffset_wh2);
  // Wrong 433.42 band is not silently treated as WH2.
  d.on_pulses(RadioId::rf43342,433420000,wh2.data(),wh2.size());
  assert(d.snapshot().fineoffset_wh2_messages==1);
  // A single valid-timing payload bit flip MUST fail the CRC.
  auto bad_wh2=wh2;
  bad_wh2[2*25]=bad_wh2[2*25]==500?1500:500;
  assert(weather433::decode(bad_wh2.data(),bad_wh2.size(),433920000).protocol==
         weather433::Protocol::none);
  d.on_pulses(RadioId::rf43392,433920000,bad_wh2.data(),bad_wh2.size());
  assert(d.snapshot().fineoffset_wh2_messages==1);
  assert(weather433::decode(wh2.data(),wh2.size()-1,433920000).protocol==
         weather433::Protocol::none);
  // v0.6.6 Acurite 609TXC: 40-bit packet, additive checksum, plausible
  // temperature/humidity, battery low bit.
  const std::vector<uint8_t> ac_bytes{0x3a,0x81,0x2c,0x41,0x28};
  auto ac=synthetic_pwm(ac_bytes,1000);
  auto direct_ac=weather433::decode(ac.data(),ac.size(),433920000);
  assert(direct_ac.protocol==weather433::Protocol::acurite_609txc &&
         direct_ac.integrity_verified && direct_ac.sensor_id==0x3a &&
         direct_ac.temperature_tenths_c==300 && direct_ac.humidity_percent==65 &&
         direct_ac.battery_low);
  d.on_pulses(RadioId::rf43392,433920000,ac.data(),ac.size());
  assert(d.snapshot().acurite_609txc_messages==1 &&
         d.snapshot().last_protocol==RecognizedProtocol::acurite_609txc);
  used=d.recent_events(events,32);
  assert(used>0 && events[used-1].kind==EventKind::acurite_609txc);
  auto bad_ac=ac;
  bad_ac[2*24]=bad_ac[2*24]==500?1000:500;
  assert(weather433::decode(bad_ac.data(),bad_ac.size(),433920000).protocol==
         weather433::Protocol::none);
  d.on_pulses(RadioId::rf43392,433920000,bad_ac.data(),bad_ac.size());
  assert(d.snapshot().acurite_609txc_messages==1);
  // Both codecs tolerate an unrelated leading mark/gap before the packet.
  auto leading_noise=ac;
  leading_noise.insert(leading_noise.begin(),{-250,125});
  assert(weather433::decode(leading_noise.data(),leading_noise.size(),433920000).protocol==
         weather433::Protocol::acurite_609txc);
  // Inverted marker mapping is supported only if the full integrity check
  // and timing contract still hold; an inverted on-air waveform is not TX.
  const auto inverted=synthetic_pwm(wh2_bytes,1500,true);
  assert(weather433::decode(inverted.data(),inverted.size(),433920000).protocol==
         weather433::Protocol::fineoffset_wh2);
  // Unrelated RMT captures and invalid channel inputs never create devices.
  std::vector<int32_t> noise(512);
  for(size_t i=0;i<noise.size();++i)noise[i]=i%2?-450:450;
  assert(weather433::decode(noise.data(),noise.size(),433920000).protocol==
         weather433::Protocol::none);
  assert(weather433::decode(noise.data(),noise.size(),868299900).protocol==
         weather433::Protocol::none);
  puts("PASS native RX decoder: Itho/RAMSES, checksum-protected WH2/Acurite receive-only, rejection paths");
}
