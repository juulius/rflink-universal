#include <cassert>
#include <iostream>
#include "rflink_universal.h"
#include "esphome/components/mqtt/mqtt_client.h"

namespace esphome { namespace mqtt { MQTTClientComponent *global_mqtt_client = nullptr; }}
using esphome::rflink_universal::RFLinkUniversal;
using esphome::rflink_universal::DeviceRecord;
using esphome::rflink_universal::DecodeResult;
using esphome::mqtt::MQTTClientComponent;

class Fixture : public RFLinkUniversal {
 public:
  void ready() { storage_ready_ = storage_ok_ = true; }
  void received(const DeviceRecord &d, const DecodeResult &r) { ha_received_(d,r); }
};
int main() {
  Fixture core;
  core.ready();
  core.set_ha_node("rflink-test");
  DeviceRecord d;
  d.uid="dev_00001"; d.name="Voortuin"; d.protocol="AlectoV1";
  d.rf_identities={"Alecto:private"}; d.fields={{"temperature","20.5","°C"},{"humidity","55","%"}};
  d.actions={"On"};
  assert(core.devices().restore_devices({d}));
  assert(!core.mqtt_configured() && !core.set_home_assistant_enabled(d.uid,true));
  MQTTClientComponent client;
  esphome::mqtt::global_mqtt_client = &client;
  assert(core.mqtt_configured() && core.mqtt_connected());
  client.fail_next=true;
  assert(!core.set_home_assistant_enabled(d.uid,true));
  assert(!core.devices().find_by_uid(d.uid)->home_assistant_enabled);
  assert(core.set_home_assistant_enabled(d.uid,true));
  assert(core.devices().find_by_uid(d.uid)->home_assistant_enabled);
  assert(client.sent.size()==3);
  for (size_t i=0;i<client.sent.size();i++) {
    const auto &p=client.sent[i];
    assert(p.retain && p.qos==1 && p.topic.find("/config")!=std::string::npos);
    assert(p.payload.find("Alecto:private")==std::string::npos);
    if(i<2) assert(p.payload.find("rflink-test/status")!=std::string::npos);
    else {assert(p.topic.find("action/config")!=std::string::npos);assert(p.payload.empty());} // Remove stale HA1 weather-button entity.
  }
  DecodeResult frame;
  frame.fields={{"temperature","22.5","°C"},{"humidity","59","%"},{"rolling_code","SECRET",""}};
  frame.action="On";
  const auto *saved=core.devices().find_by_uid(d.uid);
  core.received(*saved,frame);
  assert(client.sent.size()==5); // 2 nonretained readings; no fake sensor button event
  for (size_t i=3;i<5;i++) {
    assert(!client.sent[i].retain && client.sent[i].qos==0);
    assert(client.sent[i].payload.find("SECRET")==std::string::npos);
  }
  assert(client.sent.back().topic.find("humidity/state")!=std::string::npos);
  assert(core.rename_device(d.uid,"Achtertuin"));
  assert(client.sent.size()==8);
  for (size_t i=5;i<8;i++) {
    if (client.sent[i].topic.find("action/config")==std::string::npos)
      assert(client.sent[i].payload.find("Achtertuin")!=std::string::npos);
    assert(client.sent[i].topic.find("dev_00001")!=std::string::npos);
  }
  client.connected=false;
  assert(!core.set_home_assistant_enabled(d.uid,false));
  assert(!core.remove_device(d.uid));
  assert(core.devices().find_by_uid(d.uid)->home_assistant_enabled);
  client.connected=true;
  core.sync_home_assistant();
  assert(client.sent.size()==11);
  assert(core.set_home_assistant_enabled(d.uid,false));
  assert(core.devices().find_by_uid(d.uid) != nullptr);
  assert(!core.devices().find_by_uid(d.uid)->home_assistant_enabled);
  for (size_t i=11;i<14;i++) assert(client.sent[i].payload.empty() && client.sent[i].retain && client.sent[i].qos==1);
  assert(core.set_home_assistant_enabled(d.uid,true));
  assert(core.remove_device(d.uid));
  assert(core.devices().find_by_uid(d.uid)==nullptr);
  for (size_t i=client.sent.size()-3;i<client.sent.size();i++) assert(client.sent[i].payload.empty() && client.sent[i].retain);
  DeviceRecord unsupported; unsupported.uid="dev_00002"; unsupported.name="Unknown"; unsupported.rf_identities={"raw"};
  assert(core.devices().restore_devices({unsupported}));
  assert(!core.set_home_assistant_enabled(unsupported.uid,true));
  assert(core.devices().find_by_uid(unsupported.uid)!=nullptr);
  // HA2: a decoder may discover NEW capabilities, but ONLY user approval
  // changes the persisted topology and MQTT Discovery. Existing values stay live.
  Fixture managed;
  managed.ready();
  managed.set_ha_node("rflink-test");
  DeviceRecord original;
  original.uid="dev_00003"; original.name="Woonkamer"; original.protocol="Remote";
  original.rf_identities={"test:3"}; original.actions={"button1"};
  original.fields={{"temperature","20","°C"}};
  assert(managed.devices().restore_devices({original}));
  MQTTClientComponent bridge;
  esphome::mqtt::global_mqtt_client=&bridge;
  assert(managed.set_home_assistant_enabled(original.uid,true));
  assert(bridge.sent.size()==2);
  DecodeResult changed;
  changed.valid=true; changed.protocol="Remote"; changed.identity_key="test:3";
  changed.action="button2"; changed.expected_actions={"button3"};
  changed.fields={{"temperature","21","°C"},{"humidity","45","%"}};
  auto *known=managed.devices().observe(changed,100);
  assert(known && known->actions.size()==1 && known->fields.size()==1);
  assert(known->fields[0].value=="21");
  const auto *pending=managed.devices().pending(original.uid);
  assert(pending && pending->actions.size()==2 && pending->fields.size()==1);
  managed.received(*known,changed);
  assert(bridge.sent.size()==3 && bridge.sent.back().topic.find("temperature/state")!=std::string::npos);
  assert(bridge.sent.back().payload=="21"); // no new event or humidity state!
  assert(managed.change_capability(original.uid,"field","humidity",true));
  assert(managed.devices().find_by_uid(original.uid)->fields.size()==2);
  assert(managed.change_capability(original.uid,"action","button2",true));
  bool found_button2=false;
  for(const auto &p : bridge.sent)
    if(p.topic.find("action/config")!=std::string::npos && p.payload.find("button2")!=std::string::npos) found_button2=true;
  assert(found_button2);
  bridge.connected=false;
  const size_t before_offline=bridge.sent.size();
  assert(!managed.change_capability(original.uid,"field","humidity",false));
  assert(managed.devices().find_by_uid(original.uid)->fields.size()==2);
  assert(bridge.sent.size()==before_offline);
  bridge.connected=true;
  const size_t before_remove=bridge.sent.size();
  assert(managed.change_capability(original.uid,"field","humidity",false));
  assert(bridge.sent.size()==before_remove+3); // tombstone + remaining sensor + event update
  assert(bridge.sent[before_remove].topic.find("humidity/config")!=std::string::npos);
  assert(bridge.sent[before_remove].payload.empty() && bridge.sent[before_remove].retain);
  assert(bridge.sent[before_remove+1].topic.find("temperature/config")!=std::string::npos);
  const size_t before_action=bridge.sent.size();
  assert(managed.change_capability(original.uid,"action","button2",false));
  assert(bridge.sent.size()==before_action+2); // update event_types, no event deletion
  assert(bridge.sent.back().topic.find("action/config")!=std::string::npos);
  assert(bridge.sent.back().payload.find("button1")!=std::string::npos);
  assert(bridge.sent.back().payload.find("button2")==std::string::npos);
  const size_t last_action=bridge.sent.size();
  assert(managed.change_capability(original.uid,"action","button1",false));
  assert(bridge.sent[last_action].topic.find("action/config")!=std::string::npos);
  assert(bridge.sent[last_action].payload.empty()); // last action removes event entity
  // A later RF packet must not resurrect a manually removed action or field.
  changed.fields[0].value="23";
  known=managed.devices().observe(changed,200);
  assert(known && known->actions.empty() && known->fields.size()==1);
  assert(managed.devices().pending(original.uid)->fields.size()==1);
  const size_t received_again=bridge.sent.size();
  managed.received(*known,changed);
  assert(bridge.sent.size()==received_again+1 && bridge.sent.back().payload=="23");
  std::cout << "HA2: approval gate, live known values, unapproved events blocked, field tombstone, event-type update, offline guard and non-resurrection: PASS\n";
  std::cout << "HA bridge integration: broker errors, enable, state/event, rename, reconnect, offline block, disable, remove/tombstone, unsupported field guard: PASS\n";
}
