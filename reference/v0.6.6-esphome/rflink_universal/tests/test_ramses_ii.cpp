// Standard, real RFLink decoder integration regression from the Orcon captures.
#include "esphome/components/rflink_universal/decoder_registry.h"
#include "../decoders/ramses_ii/codec.h"
#include "ramses_ii_fixtures.h"
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>
using namespace esphome::rflink_universal;

static std::string get_field(const DecodeResult &r, const char* key) {
  for (const auto &f:r.fields) if (f.key==key) return f.value;
  return {};
}
static RFFrame as_packet(const std::vector<uint8_t> &p) {
  RFFrame rf; rf.kind=FrameKind::PACKET; rf.channel=2;
  rf.frequency_hz=868300000; rf.packet=&p; return rf;
}
int main() {
  RFDecoder *d=nullptr;
  RFDecoder *itho=nullptr;
  for (auto *candidate:decoder_registry()) {
    if (std::string(candidate->name())=="RamsesII") d=candidate;
    if (std::string(candidate->name())=="IthoRFT") itho=candidate;
  }
  assert(d && itho);
  for (int mode_case=0;mode_case<2;mode_case++) {
    const auto *bytes=mode_case?kPacket661:kPacket653;
    const size_t count=mode_case?sizeof(kPacket661):sizeof(kPacket653);
    std::vector<uint8_t> v(bytes,bytes+count);
    RFFrame rf=as_packet(v);
    assert(d->match(rf)==99);
    DecodeResult out;
    assert(d->decode(rf,out) && out.valid);
    assert(out.protocol=="RamsesII");
    assert(out.identity_key=="ramses:29:204810");
    assert(out.device_type=="ventilation" && out.action=="status");
    assert(out.identity_confidence==IdentityConfidence::HIGH);
    assert(get_field(out,"ramses_command")=="31D9");
    assert(get_field(out,"fan_mode")== (mode_case?"auto":"away"));
    assert(get_field(out,"fan_mode_code")== (mode_case?"4":"0"));
    assert(!out.transmit_supported && !out.pairing_supported);
    // In the existing firmware the top-scoring decoder wins. Verify that a
    // RAMSES frame cannot be incorrectly preferred as Itho.
    assert(d->match(rf)>itho->match(rf));
    rf.frequency_hz=433920000; assert(d->match(rf)==0);
    rf.frequency_hz=868300000; rf.kind=FrameKind::RAW;
    assert(d->match(rf)==0);
  }
  std::vector<uint8_t> cut(kPacket663,kPacket663+sizeof(kPacket663));
  RFFrame bad=as_packet(cut);
  assert(d->match(bad)==0);
  std::vector<uint8_t> garbage(63,0x00);
  RFFrame empty=as_packet(garbage);
  assert(d->match(empty)==0);
  // Check checksum and source matching guard.
  const uint8_t decoded[]={0x18,0x77,0x20,0x0A,0x77,0x20,0x0A,0x31,0xD9,0x03,0,0,4,0x95};
  rflink_ramses_ii::Frame frame;
  assert(rflink_ramses_ii::parseLogical(decoded,sizeof(decoded),frame));
  uint8_t corrupt[sizeof(decoded)];
  for(size_t i=0;i<sizeof(decoded);++i) corrupt[i]=decoded[i];
  corrupt[12]^=0x01;
  assert(!rflink_ramses_ii::parseLogical(corrupt,sizeof(corrupt),frame));
  std::printf("RAMSES-II decoderpack: PASS (#653 Away, #661 Auto, Itho priority, invalid frames, checksum)\n");
}
