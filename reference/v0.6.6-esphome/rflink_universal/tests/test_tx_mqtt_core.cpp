#include <cassert>
#include <iostream>
#include "rflink_universal.h"
#include "ha_discovery.h"
#include "esphome/components/mqtt/mqtt_client.h"
using namespace esphome;
using namespace esphome::rflink_universal;
namespace esphome {namespace mqtt { MQTTClientComponent *global_mqtt_client = nullptr; }}
class TXFixture:public RFLinkUniversal {
 public:
  void ready(){ storage_ready_=storage_ok_=true;set_raw_channel(0,nullptr,433420000,"433.42");set_raw_channel(1,nullptr,433920000,"433.92"); }
  void mark_radio(bool on){channels_[1].available=on;channels_[1].tx_ready=on;}
  void clear_cooldown(){tx_sent_=false;}
  std::string token()const{return ha_tx_session_;}
};
int main(){
  TXFixture core; core.ready(); core.set_ha_node("rflink-test");
  remote_transmitter::RemoteTransmitterComponent txradio;
  core.set_nexa_transmitter(&txradio);
  DeviceRecord d;
  d.uid="dev_00042";d.name="Lamp";d.protocol="Nexa";d.identity_confidence=IdentityConfidence::HIGH;
  d.transmit_supported=true;d.rf_identities={"nexa:012ABCD"};d.actions={"on","off"};
  d.fields={{"channel","2",""},{"group","false",""},{"level","0",""},{"tx_radio","1",""}};
  assert(core.devices().restore_devices({d}));
  mqtt::MQTTClientComponent broker;
  mqtt::global_mqtt_client=&broker;
  core.sync_home_assistant(); // subscription available; still no TX due to default false.
  assert(!core.tx_eligible(d.uid));
  assert(!core.set_transmit_enabled(d.uid,true));
  core.set_tx_controls_enabled(true);
  core.mark_radio(true);
  assert(core.tx_eligible(d.uid));
  assert(!core.set_transmit_enabled(d.uid,true)); // must first approve device for HA
  assert(core.set_home_assistant_enabled(d.uid,true));
  assert(core.set_transmit_enabled(d.uid,true));
  assert(core.devices().find_by_uid(d.uid)->transmit_enabled);
  auto saved=core.devices().find_by_uid(d.uid);
  assert(saved && saved->fields.size()==8); // 4 locked fields added to NVS-backed record
  DevicePersistence::Blob blob{};
  assert(DevicePersistence::encode(core.devices(),1,blob));
  DeviceStore rebooted;
  assert(DevicePersistence::decode(blob,rebooted));
  assert(rebooted.find_by_uid(d.uid)->transmit_enabled);
  assert(tx::eligible(*rebooted.find_by_uid(d.uid)));
  size_t button_count=0;std::string command, payload;
  for(const auto&p:broker.sent){
    if(p.topic.find("_tx_on/config")!=std::string::npos){
      assert(p.retain && p.qos==1);assert(p.payload.find("\"command_topic\"")!=std::string::npos);
      assert(p.payload.find(core.token())!=std::string::npos);button_count++;command=ha::command_topic("rflink-test",d,"on");payload=core.token();
    }
    if(p.topic.find("_tx_off/config")!=std::string::npos && !p.payload.empty())button_count++;
  }
  assert(button_count==2);
  core.mark_radio(true);
  broker.simulate_command(command,"wrong");assert(txradio.sent_count==0);
  broker.simulate_command("rflink/rflink-test/dev_00042/toggle/set",payload);assert(txradio.sent_count==0);
  broker.simulate_command("rflink/rflink-test/dev_00044/on/set",payload);assert(txradio.sent_count==0);
  broker.simulate_command(command,payload);
  assert(txradio.sent_count==1 && txradio.last_repeats==3 && txradio.last_data.device==0x12ABCD);
  assert(txradio.last_data.channel==2 && txradio.last_data.state==1);
  broker.simulate_command(command,payload);assert(txradio.sent_count==1); // cooldown
  core.clear_cooldown();core.mark_radio(false);
  broker.simulate_command(command,payload);assert(txradio.sent_count==1); // hardware offline
  core.mark_radio(true);
  // RX changes do not redirect a locked and approved TX parameter.
  auto *mutable_dev=core.devices().find_by_uid(d.uid);
  mutable_dev->fields[0].value="12";
  mutable_dev->fields[3].value="0";
  core.clear_cooldown();broker.simulate_command(command,payload);
  assert(txradio.sent_count==2 && txradio.last_data.channel==2);
  assert(!core.replace_identity(d.uid,"nexa:012ABCD","nexa:012ABC0"));
  assert(!core.unlink_identity(d.uid,"nexa:012ABCD"));
  assert(!core.change_capability(d.uid,"field","tx_lock_radio",false));
  assert(!core.change_capability(d.uid,"field","channel",false));
  // Broker down: invalidated token blocks all prior commands until re-sync.
  core.invalidate_tx_session();core.clear_cooldown();broker.simulate_command(command,payload);
  assert(txradio.sent_count==2);
  core.sync_home_assistant();assert(core.token()!=payload);
  core.clear_cooldown();broker.simulate_command(command,payload);assert(txradio.sent_count==2);
  assert(core.set_transmit_enabled(d.uid,false));
  assert(!core.devices().find_by_uid(d.uid)->transmit_enabled);
  assert(core.devices().find_by_uid(d.uid)->fields.size()==4);
  core.clear_cooldown();broker.simulate_command(command,core.token());assert(txradio.sent_count==2);
  bool tomb_on=false,tomb_off=false;
  for(const auto &p:broker.sent){
    if(p.topic.find("_tx_on/config")!=std::string::npos && p.payload.empty())tomb_on=true;
    if(p.topic.find("_tx_off/config")!=std::string::npos && p.payload.empty())tomb_off=true;
  }
  assert(tomb_on&&tomb_off);
  std::cout << "TX1 MQTT: opt-in, correct on/off Discovery, session token, invalid commands, cooldown, hardware guard, immutable RX binding, broker reconnect, disable and tombstones: PASS\n";
}
