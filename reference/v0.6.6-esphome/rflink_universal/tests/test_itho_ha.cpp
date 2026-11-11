#include <cassert>
#include <iostream>
#include <string>

#include "rflink_universal.h"
#include "ha_discovery.h"
#include "esphome/components/cc1101/cc1101.h"
#include "esphome/components/mqtt/mqtt_client.h"

using namespace esphome;
using namespace esphome::rflink_universal;
namespace esphome { ESPPreferences test_preferences; ESPPreferences *global_preferences = &test_preferences; }
namespace esphome { namespace mqtt { MQTTClientComponent *global_mqtt_client = nullptr; } }

class IthoHaCore : public RFLinkUniversal {
 public:
  void ready(cc1101::CC1101Component *radio) {
    storage_ready_ = storage_ok_ = true;
    set_packet_channel(2, 868299900, "868.30 Itho");
    set_packet_transmitter(radio);
    set_radio_available(2, true, true, true);
    set_tx_controls_enabled(true);
  }
  void no_cooldown() { tx_sent_ = false; }
  std::string session() const { return ha_tx_session_; }
};

static DeviceRecord device() {
  DeviceRecord d;
  d.uid = "dev_itho_ha";
  d.name = "Ventilatie";
  d.protocol = "IthoRFT";
  d.device_type = "ventilation_remote";
  d.identity_confidence = IdentityConfidence::HIGH;
  d.transmit_supported = true;
  d.pairing_supported = true;
  d.rf_identities = {"itho_rft:123456"};
  d.actions = {"standby", "low", "medium", "high", "timer_10", "timer_20", "timer_30"};
  return d;
}

int main() {
  mqtt::MQTTClientComponent broker;
  mqtt::global_mqtt_client = &broker;
  cc1101::CC1101Component radio;
  IthoHaCore core;
  core.ready(&radio);
  core.set_ha_node("itho-ha");
  assert(core.devices().restore_devices({device()}));

  std::string error;
  assert(core.itho_pair("dev_itho_ha", false, error));
  core.no_cooldown();
  core.sync_home_assistant();
  assert(core.set_home_assistant_enabled("dev_itho_ha", true));
  assert(core.tx_eligible("dev_itho_ha"));
  assert(core.set_transmit_enabled("dev_itho_ha", true));

  bool low=false, medium=false, high=false, t10=false, t20=false, t30=false, standby=false;
  for (const auto &msg : broker.sent) {
    if (msg.payload.empty()) continue;
    if (msg.topic.find("_tx_low/config") != std::string::npos) low=true;
    if (msg.topic.find("_tx_medium/config") != std::string::npos) medium=true;
    if (msg.topic.find("_tx_high/config") != std::string::npos) high=true;
    if (msg.topic.find("_tx_timer_10/config") != std::string::npos) t10=true;
    if (msg.topic.find("_tx_timer_20/config") != std::string::npos) t20=true;
    if (msg.topic.find("_tx_timer_30/config") != std::string::npos) t30=true;
    if (msg.topic.find("_tx_standby/config") != std::string::npos) standby=true;
  }
  assert(low && medium && high && t10 && t20 && t30 && standby);

  const auto *saved=core.devices().find_by_uid("dev_itho_ha");
  assert(saved && saved->transmit_enabled);
  const auto topic=ha::command_topic("itho-ha", *saved, "high");
  const unsigned before=radio.sent_count;
  broker.simulate_command(topic, core.session());
  assert(radio.sent_count == before + 3);

  std::cout << "Itho HA: pair first, explicit opt-in, speed/timer discovery and MQTT packet dispatch: PASS\n";
}
