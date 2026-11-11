#include <cassert>
#include <iostream>
#include <string>

#include "rflink_universal.h"
#include "itho_codec.h"
#include "tx_engine.h"
#include "esphome/components/cc1101/cc1101.h"
#include "tx_policy.h"

using namespace esphome;
using namespace esphome::rflink_universal;
namespace esphome { ESPPreferences test_preferences; ESPPreferences *global_preferences = &test_preferences; }

class IthoCore : public RFLinkUniversal {
 public:
  void ready(cc1101::CC1101Component *radio) {
    storage_ready_ = storage_ok_ = true;
    set_packet_channel(2, 868299900, "868.30 Itho");
    set_packet_transmitter(radio);
    set_radio_available(2, radio != nullptr, true, radio != nullptr);
    set_tx_controls_enabled(true);
  }
  void no_cooldown() { tx_sent_ = false; }
};

static DeviceRecord make_itho(bool with_state = false, bool paired = false) {
  DeviceRecord d;
  d.uid = "dev_itho_001";
  d.name = "Ventilatie";
  d.protocol = "IthoRFT";
  d.device_type = "ventilation_remote";
  d.identity_confidence = IdentityConfidence::HIGH;
  d.transmit_supported = true;
  d.pairing_supported = true;
  d.rf_identities = {"itho_rft:112233"};
  d.actions = {"standby", "low", "medium", "high", "timer_10", "timer_20", "timer_30"};
  if (with_state) {
    d.tx_state = {{"itho_address", "66051", ""},       // 0x010203
                  {"itho_sequence", "7", ""},
                  {"itho_device_type", "22", ""},
                  {"itho_paired", paired ? "1" : "0", ""}};
  }
  return d;
}

int main() {
  tx_engine::ensure_linked();
  const auto *encoder = tx_engine::resolve("IthoRFT");
  assert(encoder && std::string(encoder->encoder_id()) == "itho_rft");

  // Normal controls are unavailable until RFLink's virtual remote is paired.
  {
    auto d = make_itho(true, false);
    tx_engine::Plan plan;
    const tx_engine::Encoder *resolved = nullptr;
    assert(!tx_engine::prepare(d, "low", plan, resolved));
    d.tx_state.back().value = "1";
    assert(tx_engine::prepare(d, "low", plan, resolved));
    assert(plan.kind == FrameKind::PACKET && plan.radio == 2 && plan.frequency_hz == 868299900);
    assert(plan.repeats == 3 && plan.repeat_gap_ms == 40);
    assert(plan.identity == 0x010203u && plan.args[0] == static_cast<uint8_t>(itho::SpeedCommand::LOW));
    assert(plan.state_updates.size() == 1 && plan.state_updates[0].value == "8");
  }

  // Compatibility pairing reaches the alternate JOIN codec path and persists pairing.
  {
    cc1101::CC1101Component compat_radio;
    IthoCore compat_core;
    compat_core.ready(&compat_radio);
    assert(compat_core.devices().restore_devices({make_itho()}));
    std::string compat_error;
    assert(compat_core.itho_pair("dev_itho_001", true, compat_error));
    assert(compat_error.empty());
    assert(compat_radio.sent_count == 10 && compat_radio.last_packet.size() == 63);
    uint32_t compat_address = 0, compat_sequence = 0;
    uint8_t compat_type = 0;
    bool compat_paired = false;
    assert(compat_core.itho_status("dev_itho_001", compat_address, compat_sequence, compat_type, compat_paired));
    assert(compat_address != 0 && compat_sequence == 2 && compat_type == 0x16 && compat_paired);
  }

  cc1101::CC1101Component radio;
  IthoCore core;
  core.ready(&radio);
  assert(core.devices().restore_devices({make_itho()}));

  std::string error;
  assert(core.itho_pair("dev_itho_001", false, error));
  assert(error.empty());
  assert(radio.sent_count == 10 && radio.last_packet.size() == 63);

  uint32_t virtual_address = 0, next_sequence = 0;
  uint8_t device_type = 0;
  bool paired = false;
  assert(core.itho_status("dev_itho_001", virtual_address, next_sequence, device_type, paired));
  assert(virtual_address != 0 && virtual_address != 0x112233u);
  assert(next_sequence == 2 && paired && device_type == 0x16);

  core.no_cooldown();
  error.clear();
  assert(core.native_transmit("dev_itho_001", "low", error));
  assert(radio.sent_count == 13);
  itho::Decoded decoded{};
  assert(itho::decode_packet(radio.last_packet, decoded));
  assert(decoded.valid && decoded.type == itho::MessageType::SPEED);
  assert(decoded.source_id[0] == uint8_t(virtual_address >> 16u));
  assert(decoded.source_id[1] == uint8_t(virtual_address >> 8u));
  assert(decoded.source_id[2] == uint8_t(virtual_address));
  assert(decoded.counter == 2 && decoded.speed == static_cast<uint8_t>(itho::SpeedCommand::LOW));
  assert(core.itho_status("dev_itho_001", virtual_address, next_sequence, device_type, paired));
  assert(next_sequence == 3 && paired);

  core.no_cooldown();
  error.clear();
  assert(core.native_transmit("dev_itho_001", "timer_20", error));
  assert(radio.sent_count == 16);
  decoded = {};
  assert(itho::decode_packet(radio.last_packet, decoded));
  assert(decoded.valid && decoded.type == itho::MessageType::TIMER && decoded.timer_minutes == 20);
  assert(decoded.counter == 3);
  assert(core.itho_status("dev_itho_001", virtual_address, next_sequence, device_type, paired));
  assert(next_sequence == 4);

  // Device NVS keeps the virtual sender identity and monotonic sequence.
  DevicePersistence::Blob blob{};
  assert(DevicePersistence::encode(core.devices(), 86, blob));
  DeviceStore rebooted;
  assert(DevicePersistence::decode(blob, rebooted));
  const auto *saved = rebooted.find_by_uid("dev_itho_001");
  assert(saved && saved->tx_state.size() == 4);

  // Restore may move the logical counter forward but never backwards or replace the remote identity.
  error.clear();
  assert(core.restore_itho_state("dev_itho_001", virtual_address, 2, 0x16, true, error));
  assert(core.itho_status("dev_itho_001", virtual_address, next_sequence, device_type, paired));
  assert(next_sequence == 4 && paired);
  error.clear();
  assert(core.restore_itho_state("dev_itho_001", virtual_address, 20, 0x16, true, error));
  assert(core.itho_status("dev_itho_001", virtual_address, next_sequence, device_type, paired));
  assert(next_sequence == 20);
  error.clear();
  assert(!core.restore_itho_state("dev_itho_001", virtual_address ^ 1u, 30, 0x16, true, error));

  // No packet TX hardware means no sequence is consumed.
  IthoCore unavailable;
  unavailable.ready(nullptr);
  assert(unavailable.devices().restore_devices({*core.devices().find_by_uid("dev_itho_001")}));
  uint32_t addr2 = 0, seq2 = 0; uint8_t type2 = 0; bool paired2 = false;
  assert(unavailable.itho_status("dev_itho_001", addr2, seq2, type2, paired2));
  unavailable.no_cooldown();
  error.clear();
  assert(!unavailable.native_transmit("dev_itho_001", "high", error));
  assert(unavailable.itho_status("dev_itho_001", addr2, seq2, type2, paired2));
  assert(seq2 == 20);

  // Unpair uses the same virtual identity and disables normal controls afterwards.
  core.no_cooldown();
  error.clear();
  assert(core.itho_unpair("dev_itho_001", error));
  assert(radio.sent_count == 46); // 16 prior + 30 leave packets
  assert(core.itho_status("dev_itho_001", virtual_address, next_sequence, device_type, paired));
  assert(!paired && next_sequence == 21);
  core.no_cooldown();
  error.clear();
  assert(!core.native_transmit("dev_itho_001", "medium", error));

  std::cout << "Itho TX: virtual remote, standard+compat pairing, packet replay, speed/timer control, persistent monotonic sequence, unpair and radio guard: PASS\n";
}
