#include <cassert>
#include <iostream>
#include <vector>

#include "rflink_universal.h"
#ifdef USE_MQTT
#include "ha_discovery.h"
#include "esphome/components/mqtt/mqtt_client.h"
namespace esphome { namespace mqtt { MQTTClientComponent *global_mqtt_client = nullptr; }}
#endif

using namespace esphome;
using namespace esphome::rflink_universal;

class Fixture : public RFLinkUniversal {
 public:
  void ready_storage() { storage_ready_ = storage_ok_ = true; }
};

int main() {
  Fixture core;
  remote_receiver::RemoteReceiverComponent receiver;
  remote_transmitter::RemoteTransmitterComponent transmitter;
  core.set_raw_channel(0, &receiver, 433420000, "433.42");
  core.set_raw_transmitter(0, &transmitter);
  core.setup();
  core.ready_storage();
  core.set_radio_available(0, true, true, true);
  core.led_start_learning(0, false, "on");
  // Simulate separate remote presses. Timing <80ms is intentionally deduplicated
  // even when the raw pulse width differs by a couple of microseconds.
  for (int n = 0; n < 3; ++n) {
    rflink_host_millis += 120;
    remote_base::RemoteReceiveData frame({500+n, -1000, 600, -1100, 500, -1000});
    core.receive_raw(0, frame);
  }
  assert(core.developer().captures().size() == 3);
  UserDecoder recipe;
  std::string error;
  assert(UserLibrary::codebook("user_test_flow", "Flow fixture", core.developer().captures(), recipe, error));
  assert(core.save_user_decoder(recipe, error));
  assert(core.enable_user_decoder(recipe.id, true, error));

  // Editing an active decoder is transactional at the RFLinkUniversal layer:
  // a good edit stays active, while a bad edit leaves the previous recipe live.
  auto edited = recipe;
  edited.name = "Flow fixture edited";
  edited.rx_source += "# safe edit\n";
  assert(core.save_user_decoder(edited, error));
  assert(core.effective_user_decoder(recipe.id)->enabled);
  const auto good_source = core.effective_user_decoder(recipe.id)->rx_source;
  auto invalid = edited;
  invalid.rx_source = "ACTION on\nACCEPT\n";
  assert(!core.save_user_decoder(invalid, error));
  assert(core.effective_user_decoder(recipe.id)->enabled);
  assert(core.effective_user_decoder(recipe.id)->rx_source == good_source);
  recipe = *core.effective_user_decoder(recipe.id);

  // Presentation profiles are no longer restricted to user recipes.
  UserProfile builtin_profile{"nexa_remote", "Living room Nexa", "Nexa", {"on", "off"}, "Press on and off", 1};
  assert(core.save_user_profile(builtin_profile, error));
  UserProfile bad_builtin{"bad_nexa", "Bad Nexa", "Nexa", {"not_a_nexa_action"}, "", 1};
  assert(!core.save_user_profile(bad_builtin, error));

  // Immutable built-in product profiles are selectable without consuming user NVS.
  // A protocol with exactly one built-in profile may be assigned automatically.
  DecodeResult acurite;
  acurite.valid = true; acurite.protocol = "Acurite609TXC";
  acurite.identity_key = "Acurite609TXC:42"; acurite.identity_confidence = IdentityConfidence::HIGH;
  acurite.device_type = "weather_sensor"; acurite.action = "measurement";
  acurite.fields.push_back({"temperature", "20.0", "°C"});
  for (int n = 0; n < 3; ++n) core.devices().observe(acurite, 10000 + n * 1000, 1);
  const auto *official_device = core.confirm_candidate(acurite.identity_key, "AcuRite test");
  assert(official_device && official_device->profile_id == "acurite_609txc");
  const auto official_uid = official_device->uid;
  assert(core.set_user_profile(official_uid, "acurite_609txc", error));
  assert(!core.set_user_profile(official_uid, "itho_rft_5360124", error));

  UserProfile profile{"test_model", "Flow model", recipe.id, {"on"}, "Press 3x", 1};
  assert(core.save_user_profile(profile, error));
  core.led_stop_learning();
  for (int n = 0; n < 3; ++n) {
    rflink_host_millis += 1000;
    remote_base::RemoteReceiveData frame({525+n, -1000, 600, -1100, 500, -1000});
    core.receive_raw(0, frame);
  }
  if (core.decoded_frames() != 3) {
    std::cerr << "received=" << core.total_frames() << " decoded=" << core.decoded_frames()
              << " unknown=" << core.unknown_frames() << " candidates=" << core.devices().candidates().size()
              << "\n"; return 1;
  }
  assert(core.devices().candidates().size() == 1);
  const auto id = core.devices().candidates().front().identity_key;
  assert(core.devices().candidate_ready(id));
  const auto *device = core.confirm_candidate(id, "My remote");
  assert(device && device->profile_id == "test_model" && !device->local_transmit_enabled);
  const auto uid = device->uid;
  assert(!core.user_transmit(uid, "on", error));
#ifdef USE_MQTT
  mqtt::MQTTClientComponent client;
  mqtt::global_mqtt_client = &client;
  core.set_tx_controls_enabled(true);
  core.sync_home_assistant();
  assert(core.set_home_assistant_enabled(uid, true));
#endif
  assert(core.approve_user_transmit(uid, true, error));
  assert(core.user_transmit(uid, "on", error));
  assert(transmitter.sent_count == 1 && transmitter.last_repeats == 3);
  assert(transmitter.last_data.pulses.size() == 6);
  assert(!core.user_transmit(uid, "on", error)); // rate limited
#ifdef USE_MQTT
  const std::string topic = ha::button_topic("rflink-universal", *core.devices().find_by_uid(uid), "on");
  std::string session;
  for (const auto &publication : client.sent) if (publication.topic == topic && !publication.payload.empty()) {
    const auto marker = publication.payload.find("\"payload_press\":\"");
    if (marker != std::string::npos) {
      const size_t first = marker + 17;
      session = publication.payload.substr(first, publication.payload.find('"', first) - first);
    }
  }
  assert(!session.empty());
  rflink_host_millis += 1000;
  client.simulate_command(ha::command_topic("rflink-universal", *core.devices().find_by_uid(uid), "on"), session);
  assert(transmitter.sent_count == 2);
  client.simulate_command(ha::command_topic("rflink-universal", *core.devices().find_by_uid(uid), "on"), "stale-session");
  assert(transmitter.sent_count == 2);
#endif
  recipe.tx_source = "ACTEQ on\nREQUIRE\nCONST 500\nMARK\nCONST 1000\nSPACE\nSEND\n";
  assert(!core.save_user_decoder(recipe, error)); // TX binding is locked
  assert(core.approve_user_transmit(uid, false, error));
#ifdef USE_MQTT
  assert(!client.sent.empty() && client.sent.back().topic == topic && client.sent.back().payload.empty());
#endif
  assert(core.enable_user_decoder(recipe.id, false, error));
  std::cout << "User flow: capture, activate, receive, profile, TX approval, dispatch, cooldown and immutable binding PASS\n";
}
