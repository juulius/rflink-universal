#include <cassert>
#include <iostream>
#include "ha_discovery.h"
using namespace esphome::rflink_universal;
int main() {
  DeviceRecord d;
  d.uid="dev_00001"; d.name="Tuinsensor \"achter\""; d.protocol="AlectoV1";
  d.fields={{"temperature","21.4","°C"},{"humidity","55","%"},{"battery_low","false",""},
            {"rolling_code","9832",""},{"rf_id","PRIVATE",""}};
  d.actions={"On", "Off"};
  assert(ha::profile("rolling_code")==nullptr && ha::profile("rf_id")==nullptr);
  assert(ha::discovery_topic("rf board",d,"temperature")=="homeassistant/sensor/rflink_rf_board_dev_00001_temperature/config");
  const auto t=ha::field_discovery("rf board",d,*ha::profile("temperature"));
  assert(t.find("Tuinsensor \\\"achter\\\"")!=std::string::npos);
  assert(t.find("\"device_class\":\"temperature\"")!=std::string::npos);
  assert(t.find("\"unit_of_measurement\":\"°C\"")!=std::string::npos);
  assert(t.find("PRIVATE")==std::string::npos);
  const auto b=ha::field_discovery("node",d,*ha::profile("battery_low"));
  assert(b.find("\"payload_on\":\"true\"")!=std::string::npos);
  assert(!ha::has_events(d) && ha::event_discovery("node",d).empty()); // Weather packet kinds never become button events.
  d.protocol="Nexa";
  const auto e=ha::event_discovery("node",d);
  assert(ha::has_events(d) && e.find("\"event_types\":[\"On\",\"Off\"]")!=std::string::npos);
  auto before=ha::signature(d);
  d.name="Andere naam";
  assert(before!=ha::signature(d));
  std::cout<<"HA MQTT discovery: stable IDs, event types, numeric/binary sensor classes, escaping and data-minimization: PASS\n";
}
