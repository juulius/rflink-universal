#include <cassert>
#include <iostream>
#include <string>
#include <vector>
#include <array>

#include "decoder.h"
#include "decoder_registry.h"
#include "device_store.h"
#include "developer_session.h"
#include "itho_codec.h"
#include "pulse_tools.h"
#include "somfy_codec.h"

using namespace esphome::rflink_universal;

static void test_somfy() {
  somfy::Frame56 in;
  in.key = 0xA7;
  in.command = somfy::Command::UP;
  in.rolling_code = 0x1234;
  in.address = 0xA1B2C3;
  auto plain = somfy::build_plain(in);
  assert(somfy::checksum_valid(plain));
  auto wire = somfy::obfuscate(plain);
  somfy::Frame56 out;
  assert(somfy::decode_wire(wire, out));
  assert(out.key == in.key && out.rolling_code == in.rolling_code && out.address == in.address);
  assert(out.command == in.command);

  // Synthetic Manchester capture, including Somfy software sync.
  std::vector<int32_t> raw{4550, -640};
  bool last = false;
  int32_t accum = 0;
  bool first = true;
  auto push_half = [&](bool level) {
    if (first) { last = level; accum = level ? 640 : -640; first = false; return; }
    if (level == last) accum += level ? 640 : -640;
    else { raw.push_back(accum); last = level; accum = level ? 640 : -640; }
  };
  for (auto byte : wire) {
    for (int bit = 7; bit >= 0; bit--) {
      const bool v = ((byte >> bit) & 1) != 0;
      push_half(v);
      push_half(!v);
    }
  }
  if (!first) raw.push_back(accum);
  somfy::Frame56 raw_out;
  assert(somfy::decode_raw_56(raw, raw_out));
  assert(raw_out.address == in.address && raw_out.rolling_code == in.rolling_code);
}

static void test_itho() {
  const std::array<uint8_t,3> id{{0x12,0x34,0x56}};
  for (auto cmd : {itho::SpeedCommand::STANDBY, itho::SpeedCommand::LOW, itho::SpeedCommand::MEDIUM, itho::SpeedCommand::HIGH}) {
    auto payload = itho::build_speed_payload(0x16, id, 7, cmd);
    uint8_t sum = 0; for (auto b : payload) sum = static_cast<uint8_t>(sum + b); assert(sum == 0);
    auto packet = itho::encode_payload(payload, 0xAC);
    assert(packet.size() == 63);
    itho::Decoded decoded;
    assert(itho::decode_packet(packet, decoded));
    assert(decoded.source_id == id);
    assert(decoded.type == itho::MessageType::SPEED);
    assert(decoded.speed == static_cast<uint8_t>(cmd));
  }
  for (auto minutes : {10,20,30}) {
    auto packet = itho::encode_payload(itho::build_timer_payload(0x16,id,8,minutes),0xAC);
    itho::Decoded d; assert(itho::decode_packet(packet,d)); assert(d.timer_minutes == minutes);
  }
  auto j1 = itho::build_join_payload(0x16,id,9,false);
  auto j2 = itho::build_join_payload(0x16,id,10,true);
  auto leave = itho::build_leave_payload(0x16,id,11);
  uint8_t s1=0,s2=0,s3=0; for(auto b:j1)s1+=b; for(auto b:j2)s2+=b; for(auto b:leave)s3+=b;
  assert(s1==0 && s2==0 && s3==0);
  assert(j1.size()==21 && j2.size()==21 && leave.size()==15);
  assert(itho::encode_payload(j1,0xCA).size()==63);
}

static DecodeResult make_result(const std::string &id, const std::string &action="on") {
  DecodeResult r; r.valid=true; r.protocol="Test"; r.identity_key=id; r.identity_confidence=IdentityConfidence::HIGH;
  r.device_type="remote"; r.action=action; r.fields.push_back({"battery_low","false",""}); return r;
}

static void test_devices() {
  DeviceStore store(3);
  for(int i=0;i<2;i++) assert(store.observe(make_result("test:A"),100+i)==nullptr);
  assert(!store.candidate_ready("test:A"));
  store.observe(make_result("test:A"),102); assert(store.candidate_ready("test:A"));
  auto *a=store.confirm_candidate("test:A","Remote A"); assert(a); const auto uid=a->uid;
  auto *known=store.observe(make_result("test:A","off"),200); assert(known && known->uid==uid);
  // A newly observed button is a PROPOSAL, not a silently approved capability.
  assert(known->actions.size()==1 && known->actions.front()=="on");
  assert(store.pending(uid) && store.pending(uid)->actions.size()==1 && store.pending(uid)->actions.front()=="off");
  assert(store.approve_action(uid,"off"));
  assert(store.find_by_uid(uid)->actions.size()==2);
  assert(!store.approve_action(uid,"off"));
  assert(store.remove_action(uid,"off"));
  // Seeing the same deleted button again cannot re-add it without approval.
  known=store.observe(make_result("test:A","off"),210);
  assert(known->actions.size()==1 && store.pending(uid)->actions.size()==1);
  auto new_field=make_result("test:A");new_field.fields.push_back({"temperature","21.5","C"});
  known=store.observe(new_field,220);
  assert(known->fields.size()==1 && store.pending(uid)->fields.size()==1);
  assert(store.approve_field(uid,"temperature"));
  new_field.fields[1].value="22.0";
  known=store.observe(new_field,230);
  assert(known->fields.size()==2 && known->fields.back().value=="22.0");
  assert(store.remove_field(uid,"temperature"));
  assert(known->fields.size()==1);
  for(int i=0;i<3;i++) store.observe(make_result("test:B"),300+i);
  auto *cand=store.find_candidate("test:B"); assert(cand); const auto *suggest=store.suggest_similar(*cand); assert(suggest && suggest->uid==uid);
  // Never auto-merge.
  assert(store.find_by_identity("test:B")==nullptr);
  assert(store.link_candidate("test:B",uid));
  assert(store.find_by_identity("test:B") && store.find_by_identity("test:B")->uid==uid);
}

static void test_first_enrollment_approval() {
  DeviceStore store(3);
  auto r = make_result("test:partial", "on");
  r.expected_actions={"off"};
  r.fields.push_back({"temperature", "20.5", "C"});
  for (int i=0;i<3;++i) store.observe(r,100+i);
  const std::vector<std::string> actions={"on"}, fields={"temperature"};
  const auto *saved=store.confirm_candidate("test:partial","Selective",&actions,&fields);
  assert(saved && saved->actions.size()==1 && saved->actions[0]=="on");
  // The decoder's bookkeeping stays available, but a rejected visible sensor does not.
  assert(saved->fields.size()==1 && saved->fields[0].key=="temperature");
  const auto *pending=store.pending(saved->uid);
  assert(pending && pending->actions.size()==1 && pending->actions[0]=="off");
  assert(pending->fields.size()==1 && pending->fields[0].key=="battery_low");
  assert(store.approve_field(saved->uid,"battery_low"));
  assert(store.find_by_uid(saved->uid)->fields.size()==2);

  auto weather=make_result("alecto:partial","measurement");
  weather.protocol="AlectoV1";weather.device_type="sensor";
  weather.fields.push_back({"humidity","52","%"});
  for (int i=0;i<3;++i) store.observe(weather,200+i);
  const std::vector<std::string> none;
  const std::vector<std::string> temp_only={"temperature"};
  const auto *sensor=store.confirm_candidate("alecto:partial","Weather",&none,&temp_only);
  assert(sensor);
  assert(sensor->actions.size()==1 && sensor->actions[0]=="measurement");
  assert(sensor->fields.empty()); // No temperature was observed; battery/humidity were rejected.
  const auto *sensor_pending=store.pending(sensor->uid);
  assert(sensor_pending && sensor_pending->fields.size()==2);
}

static void test_developer() {
  DeveloperSession d; d.start_baseline(2);
  RFFrame f; f.channel=2; f.received_ms=10; f.kind=FrameKind::PACKET; f.has_rssi=true; f.rssi_dbm=-80;
  d.observe(0x1111,f,63); d.finish_baseline(); d.start_action(2,"boost");
  f.received_ms=20; f.rssi_dbm=-40; d.observe(0x2222,f,63); f.received_ms=22; d.observe(0x2222,f,63);
  f.received_ms=24; d.observe(0x1111,f,63);
  auto *best=d.best_new_candidate(); assert(best && best->hash==0x2222 && best->observations==2);
  DecodeResult decoded; decoded.valid=true; decoded.protocol="TestPacket"; decoded.identity_key="test:123"; decoded.action="boost";
  d.annotate(0x2222, decoded, 91);
  best=d.best_new_candidate(); assert(best && best->protocol=="TestPacket" && best->identity_key=="test:123" && best->decoder_confidence==91);
  auto snippet=d.generate_packet_decoder_snippet(std::vector<uint8_t>{0xAA,0x55}); assert(snippet.find("0xAA")!=std::string::npos);
}


static std::vector<int32_t> alecto_raw_from_bytes(const std::array<uint8_t,5> &b) {
  std::vector<int32_t> raw;
  raw.reserve(72);
  for (size_t bit=0; bit<36; bit++) {
    bool one = (b[bit/8] & (0x80u >> (bit%8))) != 0;
    raw.push_back(500);
    raw.push_back(one ? -4000 : -2000);
  }
  return raw;
}

static void test_alecto() {
  RFDecoder *decoder = nullptr;
  for (auto *d : decoder_registry()) if (d && std::string(d->name()) == "AlectoV1") decoder = d;
  assert(decoder != nullptr);
  // Use a deterministic frame and brute-force only the checksum nibble, exactly
  // like a captured transmitter would provide it.
  std::array<uint8_t,5> b{{0xA4, 0x10, 0x32, 0x45, 0x00}};
  bool found = false;
  DecodeResult decoded;
  for (uint8_t nibble=0; nibble<16 && !found; nibble++) {
    b[4] = static_cast<uint8_t>(nibble << 4u);
    auto raw = alecto_raw_from_bytes(b);
    RFFrame f; f.kind=FrameKind::RAW; f.channel=1; f.frequency_hz=433920000; f.raw=&raw;
    if (decoder->match(f) >= 60 && decoder->decode(f, decoded)) found = true;
  }
  assert(found && decoded.valid && decoded.protocol == "AlectoV1");
  assert(decoded.identity_confidence == IdentityConfidence::MEDIUM);
  assert(!decoded.fields.empty());
}

static void test_registry() {
  bool somfy_found=false, itho_found=false, alecto_found=false;
  for(auto *d: decoder_registry()) {
    if (!d) continue;
    std::string n = d->name();
    somfy_found |= n=="SomfyRTS56"; itho_found |= n=="IthoRFT"; alecto_found |= n=="AlectoV1";
  }
  assert(somfy_found && itho_found && alecto_found);
}

int main() {
  test_somfy(); test_itho(); test_devices(); test_first_enrollment_approval(); test_developer(); test_alecto(); test_registry();
  std::cout << "RFLink Universal host tests: PASS\n";
  return 0;
}
