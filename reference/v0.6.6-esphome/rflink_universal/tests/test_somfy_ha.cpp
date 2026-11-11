#include <cassert>
#include <iostream>
#include <string>

#include "rflink_universal.h"
#include "ha_discovery.h"
#include "esphome/components/mqtt/mqtt_client.h"

using namespace esphome;
using namespace esphome::rflink_universal;
namespace esphome { namespace mqtt { MQTTClientComponent *global_mqtt_client = nullptr; } }

class SomfyHaCore : public RFLinkUniversal {
 public:
  void ready() {
    storage_ready_ = storage_ok_ = true;
    set_raw_channel(0, nullptr, 433420000, "433.42");
    channels_[0].available = true;
    channels_[0].tx_ready = true;
    set_tx_controls_enabled(true);
  }
  void no_cooldown() { tx_sent_ = false; }
  std::string session() const { return ha_tx_session_; }
};

static DeviceRecord device() {
  DeviceRecord d;
  d.uid = "dev_somfy_ha";
  d.name = "Blind";
  d.protocol = "SomfyRTS56";
  d.device_type = "cover_remote";
  d.identity_confidence = IdentityConfidence::HIGH;
  d.transmit_supported = true;
  d.pairing_supported = true;
  d.rf_identities = {"somfy_rts:123456"};
  d.actions = {"up", "down", "my", "prog"};
  return d;
}

int main() {
  mqtt::MQTTClientComponent broker;
  mqtt::global_mqtt_client = &broker;
  SomfyHaCore core;
  core.ready();
  core.set_ha_node("somfy-ha");
  remote_transmitter::RemoteTransmitterComponent radio;
  core.set_raw_transmitter(0, &radio);
  assert(core.devices().restore_devices({device()}));

  std::string error;
  assert(core.somfy_pair("dev_somfy_ha", error));
  core.no_cooldown();
  core.sync_home_assistant();
  assert(core.set_home_assistant_enabled("dev_somfy_ha", true));
  assert(core.tx_eligible("dev_somfy_ha"));
  assert(core.set_transmit_enabled("dev_somfy_ha", true));

  bool up = false, down = false, my = false, prog = false;
  for (const auto &msg : broker.sent) {
    if (msg.payload.empty()) continue;
    if (msg.topic.find("_tx_up/config") != std::string::npos) up = true;
    if (msg.topic.find("_tx_down/config") != std::string::npos) down = true;
    if (msg.topic.find("_tx_my/config") != std::string::npos) my = true;
    if (msg.topic.find("_tx_prog/config") != std::string::npos) prog = true;
  }
  assert(up && down && my);
  assert(!prog);  // Pairing is never published as an ordinary HA button.

  const auto *saved = core.devices().find_by_uid("dev_somfy_ha");
  assert(saved && saved->transmit_enabled);
  const auto topic = ha::command_topic("somfy-ha", *saved, "up");
  const unsigned before = radio.sent_count;
  broker.simulate_command(topic, core.session());
  assert(radio.sent_count == before + 1);

  std::cout << "Somfy HA: pair first, explicit opt-in, Up/My/Down discovery, no PROG button and MQTT dispatch: PASS\n";
}
