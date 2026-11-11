// Independent deterministic protocol fixtures for the three added weather decoders.
// Test both the *actual registered decoder objects* and false-positive rejection.
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include "decoder_registry.h"

using namespace esphome::rflink_universal;

static RFDecoder *named(const char *name) {
  for (auto *d : decoder_registry()) if (d && std::string(d->name()) == name) return d;
  return nullptr;
}

static uint8_t crc8(const uint8_t *p, size_t n, uint8_t seed) {
  uint8_t crc=seed;
  for (size_t i=0;i<n;++i) {
    crc ^= p[i];
    for (unsigned k=0;k<8;++k) crc = (crc & 128u) ? static_cast<uint8_t>((crc<<1u)^0x31u) : static_cast<uint8_t>(crc<<1u);
  }
  return crc;
}

static std::vector<int32_t> ppm(const std::vector<bool> &bits) {
  std::vector<int32_t> raw;
  raw.reserve(bits.size()*2);
  for (bool b : bits) {raw.push_back(500);raw.push_back(b ? -2000 : -1000);}
  return raw;
}
static std::vector<bool> bits(const uint8_t *bytes, size_t nbits) {
  std::vector<bool> r;
  for (size_t i=0;i<nbits;++i) r.push_back((bytes[i/8] & (0x80u>>(i%8)))!=0);
  return r;
}
static std::vector<int32_t> pwm(const uint8_t *bytes, size_t nbits, bool long_for_one, int32_t long_us=1500) {
  std::vector<int32_t> raw;
  for (bool b : bits(bytes, nbits)) {raw.push_back((b==long_for_one)?long_us:500);raw.push_back(-1000);}
  return raw;
}
static RFFrame make_frame(const std::vector<int32_t> &v, uint32_t f=433920000) {
  RFFrame r; r.kind=FrameKind::RAW; r.channel=1; r.frequency_hz=f;r.raw=&v;return r;
}
static std::string field(const DecodeResult &r, const std::string &name) {
  for (const auto &f : r.fields) if (f.key==name) return f.value;
  return "missing";
}
static void negative_guards(RFDecoder *d, const std::vector<int32_t> &valid) {
  assert(d);
  RFFrame wrong_frequency = make_frame(valid, 868300000);
  assert(d->match(wrong_frequency)==0);
  wrong_frequency.frequency_hz=433420000;
  assert(d->match(wrong_frequency)==0);
  RFFrame wrong_mode=make_frame(valid); wrong_mode.kind=FrameKind::PACKET;
  assert(d->match(wrong_mode)==0);
  std::vector<int32_t> short_frame(valid.begin(),valid.begin()+valid.size()/2);
  RFFrame short_rf=make_frame(short_frame);
  assert(d->match(short_rf)==0);
  auto reversed=valid; for (auto &v : reversed) v=-v;
  RFFrame inv=make_frame(reversed);
  assert(d->match(inv)==0);
}
static void acurite() {
  auto *d=named("Acurite609TXC"); assert(d);
  std::array<uint8_t,5> payload{{0x51,0x21,0xea,55,0}}; // 0x1ea = +49.0 C
  // Use +21.3 C instead.
  payload[1]=0x20|0x0; payload[2]=0xd5;
  payload[4]=static_cast<uint8_t>(payload[0]+payload[1]+payload[2]+payload[3]);
  auto raw=pwm(payload.data(),40,true,1000);
  auto rf=make_frame(raw);DecodeResult out;
  assert(d->match(rf)>=80 && d->decode(rf,out));
  assert(out.valid && out.identity_key=="acurite609:51");
  assert(field(out,"temperature")=="21.3" && field(out,"humidity")=="55");
  assert(!out.transmit_supported && out.identity_confidence==IdentityConfidence::MEDIUM);
  negative_guards(d,raw);
  // A corruption changes exactly one data bit and preserves waveform length.
  auto bad=raw;bad[32]=(bad[32]==1000?500:1000);auto brf=make_frame(bad);
  assert(d->match(brf)==0);
  // Identical telemetry, other random ID: no accidental merge.
  payload[0]=0x52;payload[4]=static_cast<uint8_t>(payload[0]+payload[1]+payload[2]+payload[3]);
  auto second=pwm(payload.data(),40,true,1000); auto srf=make_frame(second);DecodeResult out2;
  assert(d->decode(srf,out2) && out2.identity_key!=out.identity_key);
}
static void rubicson() {
  auto *d=named("RubicsonTemp");assert(d);
  // 0x7b = +12.3 C, F nibble marker, CRC8 poly 0x31 init 0x6c.
  uint8_t bytes[5]={0x5a,0x90,0x7b,0xf0,0};
  uint8_t content[5]={bytes[0],bytes[1],bytes[2],0xf0,0};
  for(unsigned candidate=0;candidate<256;++candidate) {
    content[4]=static_cast<uint8_t>(candidate);
    if(crc8(content,5,0x6c)==0){bytes[3]|=static_cast<uint8_t>(candidate>>4);bytes[4]=static_cast<uint8_t>(candidate<<4);break;}
  }
  auto raw=ppm(bits(bytes,36));auto rf=make_frame(raw);DecodeResult out;
  assert(d->match(rf)>=90 && d->decode(rf,out));
  assert(out.identity_key=="rubicson:169" && field(out,"temperature")=="12.3");
  assert(field(out,"channel")=="2" && !out.transmit_supported);
  negative_guards(d,raw);
  auto bad=raw;bad[8]=(bad[8]==-2000?-1000:-2000);auto brf=make_frame(bad);
  assert(d->match(brf)==0);
}
static void fineoffset() {
  auto *d=named("FineOffsetWH2");assert(d);
  // 48 bits: FF then four payload bytes followed by CRC (5 bytes after FF).
  std::array<uint8_t,6> bytes{{0xff,0x4a,0xb1,0x41,64,0}};
  bytes[5]=crc8(bytes.data()+1,4,0);
  auto raw=pwm(bytes.data(),48,true);auto rf=make_frame(raw);DecodeResult out;
  assert(d->match(rf)>=90 && d->decode(rf,out));
  assert(out.identity_key=="fineoffset_wh2:AB");
  assert(field(out,"temperature")=="32.1" && field(out,"humidity")=="64");
  assert(!out.transmit_supported);
  negative_guards(d,raw);
  auto bad=raw;bad[50]=(bad[50]==1500?500:1500);auto brf=make_frame(bad);
  assert(d->match(brf)==0);
  // Header damaged, checksum still matches payload but must be rejected.
  bad=raw;bad[0]=500;brf=make_frame(bad);assert(d->match(brf)==0);
}
int main() {assert(decoder_registry().size()>=7); acurite();rubicson();fineoffset();std::cout<<"Three weather decoders: valid frames, fields, ID, checksum/CRC, band, mode, truncated, reversed and tampered: PASS\n";}
