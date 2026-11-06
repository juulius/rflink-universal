#include "rflink_mqtt.hpp"
#include <cassert>
#include <cstring>
#include <string>
int main(){
  rflink::MqttSettings s{};
  assert(!rflink::mqtt_valid_settings(s));
  strcpy(s.host,"192.168.1.50");
  assert(rflink::mqtt_valid_settings(s));
  strcpy(s.host,"core-mosquitto");
  assert(rflink::mqtt_valid_settings(s));
  strcpy(s.host,"bad/path");
  assert(!rflink::mqtt_valid_settings(s));
  strcpy(s.host,"broker.local");s.tls=1;
  assert(!rflink::mqtt_valid_settings(s));
  s.tls=0;s.port=0;
  assert(!rflink::mqtt_valid_settings(s));
  s.port=1883;
  assert(rflink::mqtt_valid_settings(s));
  uint8_t mac[6]={0x01,0x23,0x45,0x67,0x89,0xab};
  assert(rflink::mqtt_hub_id(mac)=="rflink_0123456789ab");
  assert(rflink::mqtt_hub_id(mac)=="rflink_0123456789ab");
}
