#include <cassert>
#include <iostream>
#include "rflink_universal.h"
#include "tx_engine.h"
#include "ha_discovery.h"
#include "esphome/components/mqtt/mqtt_client.h"
using namespace esphome;
using namespace esphome::rflink_universal;
namespace esphome {namespace mqtt { MQTTClientComponent *global_mqtt_client=nullptr; }}
class TestCore:public RFLinkUniversal {
 public:
  void ready(){storage_ready_=storage_ok_=true;set_raw_channel(0,nullptr,433420000,"433.42");set_raw_channel(1,nullptr,433920000,"433.92");}
  void radio(bool up){channels_[1].available=up;channels_[1].tx_ready=up;}
  void no_cooldown(){tx_sent_=false;}
  std::string token()const{return ha_tx_session_;}
};
static DeviceRecord make_byron(const std::string &action="command_2") {
  DeviceRecord d;d.uid="dev_00073";d.protocol="ByronSX";d.name="Deurbel";
  d.identity_confidence=IdentityConfidence::HIGH;d.transmit_supported=true;
  d.rf_identities={"byronsx:4F"};d.actions={action};d.fields={{"tx_radio","1",""}};
  return d;
}
int main() {
  tx_engine::ensure_linked(); tx_engine::ensure_linked();
  assert(tx_engine::resolve("Nexa") && tx_engine::resolve("ByronSX"));
  assert(tx_engine::resolve("Dooya") && tx_engine::resolve("RCSwitch"));
  assert(tx_engine::resolve("SomfyRTS56"));
  assert(!tx_engine::resolve("KeeLoq") && tx_engine::resolve("IthoRFT"));
  tx_engine::Plan p;const tx_engine::Encoder *encoder=nullptr;
  auto d=make_byron();
  assert(tx_engine::prepare(d,"command_2",p,encoder) && p.radio==1 && p.frequency_hz==433920000 && p.identity==0x4f && p.args[0]==2);
  assert(!tx_engine::prepare(d,"command_3",p,encoder)); // action not approved by user
  assert(!tx_engine::prepare(d,"command_16",p,encoder)); // wildcard command isn't a TX code
  d.actions={"command_16"};assert(!tx_engine::eligible(d));
  d=make_byron("command_02");assert(!tx_engine::eligible(d)); // canonical action only
  d=make_byron();d.fields[0].value="0";assert(!tx_engine::eligible(d));
  d=make_byron();d.rf_identities.push_back("byronsx:50");assert(!tx_engine::eligible(d));
  d=make_byron();d.identity_confidence=IdentityConfidence::LOW;assert(!tx_engine::eligible(d));
  d=make_byron();assert(tx_engine::eligible(d));
  mqtt::MQTTClientComponent broker;mqtt::global_mqtt_client=&broker;
  TestCore core;core.ready();core.set_ha_node("tx2-test");core.set_tx_controls_enabled(true);
  remote_transmitter::RemoteTransmitterComponent tx433;
  core.set_raw_transmitter(1,&tx433);
  assert(core.devices().restore_devices({d}));core.sync_home_assistant();
  core.radio(true);
  assert(core.tx_eligible(d.uid));assert(core.set_home_assistant_enabled(d.uid,true));
  assert(core.set_transmit_enabled(d.uid,true));
  const auto *saved=core.devices().find_by_uid(d.uid);
  assert(saved && saved->transmit_enabled && saved->fields.size()==2);
  assert(saved->fields[1].key=="tx_lock_radio");
  DevicePersistence::Blob blob{};assert(DevicePersistence::encode(core.devices(),7,blob));
  DeviceStore restored;assert(DevicePersistence::decode(blob,restored));
  assert(restored.find_by_uid(d.uid)->transmit_enabled && tx_engine::eligible(*restored.find_by_uid(d.uid)));
  const auto topic=ha::command_topic("tx2-test",d,"command_2");const auto key=core.token();
  bool discovery=false;
  for(const auto &m:broker.sent) if(m.topic.find("_tx_command_2/config")!=std::string::npos && !m.payload.empty()) {
    discovery=true;assert(m.payload.find("\"name\":\"command_2\"")!=std::string::npos);
  }
  assert(discovery);
  broker.simulate_command(topic,"bad");assert(tx433.sent_count==0);
  broker.simulate_command("rflink/tx2-test/dev_00073/command_9/set",key);assert(tx433.sent_count==0);
  core.radio(true);broker.simulate_command(topic,key);
  assert(tx433.sent_count==1 && tx433.last_data.address==0x4f && tx433.last_data.command==2 && tx433.last_repeats==3);
  core.no_cooldown();core.radio(false);broker.simulate_command(topic,key);assert(tx433.sent_count==1);
  core.radio(true);assert(!core.unlink_identity(d.uid,"byronsx:4F"));
  assert(!core.change_capability(d.uid,"field","tx_lock_radio",false));
  core.no_cooldown();assert(core.set_transmit_enabled(d.uid,false));
  broker.simulate_command(topic,key);assert(tx433.sent_count==1);
  bool tomb=false;for(const auto &m:broker.sent) if(m.topic.find("_tx_command_2/config")!=std::string::npos && m.payload.empty()) tomb=true;
  assert(tomb);
  std::cout<<"TX2: decoder registry, Byron 4-bit fixed commands, immutable radio, NVS, HA Discovery, opt-in, hardware guard, tombstones: PASS\n";
}
