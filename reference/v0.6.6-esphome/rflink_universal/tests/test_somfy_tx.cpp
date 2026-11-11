#include <cassert>
#include <iostream>

#include "rflink_universal.h"
#include "somfy_codec.h"
#include "tx_engine.h"
#include "tx_policy.h"

using namespace esphome;
using namespace esphome::rflink_universal;

class SomfyCore : public RFLinkUniversal {
 public:
  void ready() {
    storage_ready_ = storage_ok_ = true;
    set_raw_channel(0, nullptr, 433420000, "433.42");
    channels_[0].available = true;
    channels_[0].tx_ready = true;
    set_tx_controls_enabled(true);
  }
  void no_cooldown() { tx_sent_ = false; }
};

static DeviceRecord make_somfy(bool with_state = false, bool paired = false) {
  DeviceRecord d;
  d.uid = "dev_00560";
  d.name = "Somfy blind";
  d.protocol = "SomfyRTS56";
  d.identity_confidence = IdentityConfidence::HIGH;
  d.transmit_supported = true;
  d.rf_identities = {"somfy_rts:112233"};
  d.actions = {"up", "down", "my", "prog"};
  if (with_state) {
    d.tx_state = {{"somfy_address", "66051", ""},       // 0x010203
                  {"somfy_rolling_code", "7", ""},
                  {"somfy_paired", paired ? "1" : "0", ""}};
  }
  return d;
}

int main() {
  tx_engine::ensure_linked();
  const auto *encoder = tx_engine::resolve("SomfyRTS56");
  assert(encoder && std::string(encoder->encoder_id()) == "somfy_rts_56");

  // A normal movement may never be encoded before this virtual RFLink remote
  // has been paired. PROG is the only pre-pair command.
  {
    auto d = make_somfy(true, false);
    tx_engine::Plan plan;
    const tx_engine::Encoder *resolved = nullptr;
    assert(!tx_engine::prepare(d, "up", plan, resolved));
    assert(tx_engine::prepare(d, "prog", plan, resolved));
    assert(plan.radio == 0 && plan.frequency_hz == 433420000);
    assert(plan.identity == 0x010203u && plan.args[3] == 0xA7);
    assert(plan.state_updates.size() == 2);
  }

  SomfyCore core;
  core.ready();
  remote_transmitter::RemoteTransmitterComponent radio;
  core.set_raw_transmitter(0, &radio);
  assert(core.devices().restore_devices({make_somfy()}));

  std::string error;
  assert(core.somfy_pair("dev_00560", error));
  assert(error.empty());
  assert(radio.sent_count == 1 && radio.last_repeats == 1);
  assert(!radio.last_data.pulses.empty());
  assert(radio.last_data.pulses.front() == 9415);
  assert(radio.last_data.pulses.size() > 100);

  uint32_t virtual_address = 0;
  uint16_t next_counter = 0;
  bool paired = false;
  assert(core.somfy_status("dev_00560", virtual_address, next_counter, paired));
  assert(virtual_address != 0 && virtual_address != 0x112233u);
  assert(next_counter == 2 && paired);

  // Decode the generated on-air frame with the existing RX codec: pairing
  // must have used the just-created virtual identity and counter 1.
  somfy::Frame56 decoded{};
  assert(somfy::decode_raw_56(radio.last_data.pulses, decoded));
  assert(decoded.address == virtual_address);
  assert(decoded.command == somfy::Command::PROG);
  assert(decoded.rolling_code == 1);

  // A normal command consumes exactly one rolling code, even though the
  // waveform contains repeated frames.
  core.no_cooldown();
  error.clear();
  assert(core.native_transmit("dev_00560", "up", error));
  assert(radio.sent_count == 2 && radio.last_repeats == 1);
  assert(somfy::decode_raw_56(radio.last_data.pulses, decoded));
  assert(decoded.address == virtual_address);
  assert(decoded.command == somfy::Command::UP);
  assert(decoded.rolling_code == 2);
  assert(core.somfy_status("dev_00560", virtual_address, next_counter, paired));
  assert(next_counter == 3 && paired);

  // The protocol-owned transmitter state survives the normal device NVS blob.
  DevicePersistence::Blob blob{};
  assert(DevicePersistence::encode(core.devices(), 56, blob));
  DeviceStore rebooted;
  assert(DevicePersistence::decode(blob, rebooted));
  const auto *saved = rebooted.find_by_uid("dev_00560");
  assert(saved && saved->tx_state.size() == 3);
  uint32_t restored_counter = 0;
  const auto *counter = tx::state(*saved, "somfy_rolling_code");
  assert(counter && tx::unsigned_decimal(*counter, 0xFFFFu, restored_counter));
  assert(restored_counter == 3);

  // Backup restore may never move the rolling counter backwards and may never
  // replace an already configured virtual remote with another address.
  error.clear();
  assert(core.restore_somfy_state("dev_00560", virtual_address, 2, true, error));
  assert(core.somfy_status("dev_00560", virtual_address, next_counter, paired));
  assert(next_counter == 3 && paired);
  error.clear();
  assert(!core.restore_somfy_state("dev_00560", virtual_address ^ 1u, 20, true, error));
  assert(core.somfy_status("dev_00560", virtual_address, next_counter, paired));
  assert(next_counter == 3 && paired);

  // Missing TX hardware must not consume another counter.
  SomfyCore unavailable;
  unavailable.ready();
  assert(unavailable.devices().restore_devices({*saved}));
  error.clear();
  assert(!unavailable.native_transmit("dev_00560", "down", error));
  uint32_t addr2 = 0; uint16_t counter2 = 0; bool paired2 = false;
  assert(unavailable.somfy_status("dev_00560", addr2, counter2, paired2));
  assert(addr2 == virtual_address && counter2 == 3 && paired2);

  std::cout << "Somfy TX: virtual remote, pairing PROG, 56-bit waveform replay, persistent monotonic rolling code and radio guard: PASS\n";
}
