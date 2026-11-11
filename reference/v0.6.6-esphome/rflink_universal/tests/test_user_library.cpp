#include <cassert>
#include <iostream>
#include <memory>

#include "user_library.h"

using namespace esphome::rflink_universal;

int main() {
  UserLibrary library;
  UserDecoder d;
  d.id = "user_custom_switch";
  d.name = "My switch";
  d.radio = 1;
  d.kind = FrameKind::RAW;
  d.actions = {"on"};
  d.rx_source = "LEN\nCONST 4\nEQ\nREQUIRE\nPULSE 0\nABS\nCONST 500\nSUB\nABS\nCONST 100\nLT\nREQUIRE\nCONST 2\nID\nACTION on\nACCEPT";
  d.tx_source = "ACTEQ on\nREQUIRE\nCONST 500\nMARK\nCONST 1000\nSPACE\nCONST 500\nMARK\nCONST 1000\nSPACE\nREPEATS 3\nSEND";
  for (int i = 0; i < 3; ++i) {
    CapturedFrame sample;
    sample.kind = FrameKind::RAW;
    sample.channel = 1;
    sample.label = "on";
    sample.raw = {500 + i * 10, -1000, 500, -1000};
    d.samples.push_back(sample);
  }
  std::string error;
  if (!library.upsert_decoder(d, error)) { std::cerr << error << '\n'; return 1; }
  assert(library.enable_decoder("user_custom_switch", true, error));
  assert(library.decoder("user_custom_switch")->enabled);
  RFFrame rx;
  std::vector<int32_t> raw{490, -1000, 500, -1000};
  rx.kind = FrameKind::RAW; rx.channel = 1; rx.raw = &raw;
  DecodeResult decoded;
  assert(user_vm::receive(library.decoder("user_custom_switch")->rx, "user_custom_switch", rx, decoded, error));
  assert(decoded.action == "on" && decoded.identity_key == "2");
  raw[0] = 350;
  assert(!user_vm::receive(library.decoder("user_custom_switch")->rx, "user_custom_switch", rx, decoded, error));

  user_vm::TxFrame tx;
  assert(user_vm::transmit(library.decoder("user_custom_switch")->tx, "on", 1, {}, tx, error));
  assert(tx.radio == 1 && tx.repeats == 3 && tx.raw.size() == 4 && tx.packet.empty());
  assert(!user_vm::transmit(library.decoder("user_custom_switch")->tx, "off", 1, {}, tx, error));

  auto blob = std::make_unique<UserLibraryPersistence::Blob>();
  assert(UserLibraryPersistence::encode(library, 1, *blob));
  assert(UserLibraryPersistence::valid(*blob));
  UserLibrary restored;
  assert(UserLibraryPersistence::decode(*blob, restored));
  assert(restored.decoder("user_custom_switch") && restored.decoder("user_custom_switch")->enabled);
  assert(restored.decoder("user_custom_switch")->version == 1);
  blob->bytes[1] ^= 0xFF;
  assert(!UserLibraryPersistence::valid(*blob));

  UserProfile profile{"my_model", "My button", "user_custom_switch", {"on"}, "Press once", 1};
  assert(restored.upsert_profile(profile, error));
  assert(restored.upsert_profile(profile, error));
  auto updated = *restored.decoder("user_custom_switch");
  updated.tx_source += "\n# changed TX rule";
  assert(!restored.upsert_decoder(updated, error)); // active rules require an explicit disable
  updated = *restored.decoder("user_custom_switch");
  updated.name = "Renamed switch";
  assert(restored.upsert_decoder(updated, error));
  assert(restored.commit_state({{"counter", 1}}));
  const std::string long_key = "dev_00001:" + std::string(32, 'd') + ":" + std::string(32, 'c');
  assert(restored.commit_state({{long_key, 1}}));
  assert(!restored.commit_state({{"counter", 1}}));
  auto second = std::make_unique<UserLibraryPersistence::Blob>();
  assert(UserLibraryPersistence::encode(restored, 2, *second));
  UserLibrary final;
  assert(UserLibraryPersistence::decode(*second, final));
  assert(final.profiles().size() == 1 && final.state().at("counter") == 1 && final.state().at(long_key) == 1);
  assert(final.decoder("user_custom_switch")->version == 2);
  assert(final.decoder("user_custom_switch")->enabled);
  assert(final.profiles().front().version == 2);

  // A beginner can activate an entirely new two-button protocol from the UI.
  std::vector<CapturedFrame> captures;
  for (const std::string action : {"on", "off"}) for (int n = 0; n < 3; ++n) {
    CapturedFrame example;
    example.channel = 0; example.kind = FrameKind::RAW; example.label = action;
    example.raw = {500+n*5, -1000, action == "on" ? 700 : 1400,
                   -1100, 500, -1000};
    captures.push_back(example);
  }
  UserDecoder generated;
  if (!UserLibrary::codebook("user_test_switch", "Test switch", captures, generated, error)) {
    std::cerr << "codebook error: " << error << '\n'; return 1;
  }
  assert(!generated.tx_source.empty() && generated.actions.size() == 2);
  assert(final.upsert_decoder(generated, error));
  assert(final.enable_decoder(generated.id, true, error));
  auto off = captures.back().raw;
  rx.channel = 0; rx.raw = &off;
  assert(user_vm::receive(final.decoder(generated.id)->rx, generated.id, rx, decoded, error));
  assert(decoded.action == "off");
  assert(!UserLibrary::codebook("user_insufficient", "Bad", {captures[0],captures[1]}, generated, error));

  // USERLIB2: fixed RAW telegrams substantially longer than the old ~70-pulse
  // ceiling fit because MARKUS/SPACEUS use one bounded-VM instruction each.
  std::vector<CapturedFrame> long_captures;
  for (int n = 0; n < 3; ++n) {
    CapturedFrame sample;
    sample.channel = 1; sample.kind = FrameKind::RAW; sample.label = "on";
    for (int i = 0; i < 120; ++i) sample.raw.push_back((i % 2 == 0 ? 1 : -1) * (500 + (i % 6) * 100 + n));
    long_captures.push_back(sample);
  }
  assert(UserLibrary::codebook("user_long_raw", "Long RAW", long_captures, generated, error));
  assert(!generated.tx_source.empty());
  assert(generated.tx_source.find("MARKUS ") != std::string::npos);
  assert(generated.tx_source.find("SPACEUS ") != std::string::npos);
  assert(generated.tx.code.size() <= user_vm::MAX_INSTRUCTIONS);
  assert(user_vm::transmit(generated.tx, "on", 1, {}, tx, error));
  assert(tx.raw.size() == 120);

  std::vector<CapturedFrame> changing_packets;
  for (int n = 0; n < 3; ++n) {
    CapturedFrame sample;
    sample.channel = 2; sample.kind = FrameKind::PACKET; sample.label = "on";
    sample.packet.assign(63, 42);
    sample.packet[20] = uint8_t(n + 1); // a changing counter must disable generated replay
    changing_packets.push_back(sample);
  }
  assert(UserLibrary::codebook("user_counter", "Changing packet", changing_packets, generated, error));
  assert(generated.tx_source.empty());

  user_vm::Program program;
  assert(!user_vm::compile("JMP 0\nACCEPT", user_vm::Mode::RX, program, error) ||
         !user_vm::receive(program, "loop", rx, decoded, error));
  assert(!user_vm::compile("CONST 1\nSEND", user_vm::Mode::RX, program, error));
  assert(!user_vm::compile("EMIT_BYTE\nSEND", user_vm::Mode::TX, program, error) ||
         !user_vm::transmit(program, "on", 2, {}, tx, error));
  std::cout << "User library: replay, RX, TX, state, validation and persistence PASS\n";
}
