#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <functional>
namespace esphome { namespace mqtt {
struct Published { std::string topic, payload; uint8_t qos; bool retain; };
class MQTTClientComponent {
 public:
  // Minimal 2026.10 MQTT runtime setter surface (the real ESPHome client has these).
  std::string broker,username,password;uint16_t port{1883};bool boot_enabled{false};
  void set_broker_address(const std::string &v){broker=v;}
  void set_broker_port(uint16_t v){port=v;}
  void set_username(const std::string &v){username=v;}
  void set_password(const std::string &v){password=v;}
  void set_enable_on_boot(bool v){boot_enabled=v;}
  void enable(){connected=true;}
  void disable(){connected=false;}
  bool connected{true};
  bool fail_next{false};
  std::vector<Published> sent;
  std::vector<std::pair<std::string, std::function<void(const std::string&,const std::string&)>>> subscriptions;
  void subscribe(const std::string& topic, std::function<void(const std::string&,const std::string&)> callback, uint8_t qos=0) {
    (void)qos; subscriptions.emplace_back(topic,std::move(callback));
  }
  void simulate_command(const std::string& topic,const std::string& payload) {
    for(const auto &sub:subscriptions) {
      const auto &filter=sub.first;
      size_t fp=0,tp=0;bool match=true;
      while(fp<filter.size() || tp<topic.size()) {
        auto fe=filter.find('/',fp),te=topic.find('/',tp);
        if(fe==std::string::npos) fe=filter.size();
        if(te==std::string::npos) te=topic.size();
        if(!(fe-fp==1 && filter[fp]=='+') && filter.compare(fp,fe-fp,topic,tp,te-tp)!=0) match=false;
        if(!match || (fe==filter.size()) != (te==topic.size())) { match=false;break; }
        if(fe==filter.size()) break;
        fp=fe+1;tp=te+1;
      }
      if(match) sub.second(topic,payload);
    }
  }
  bool is_connected() const { return connected; }
  bool publish(const std::string &topic, const std::string &payload, uint8_t qos = 0, bool retain = false) {
    if (!connected || fail_next) { fail_next = false; return false; }
    sent.push_back({topic,payload,qos,retain}); return true;
  }
};
extern MQTTClientComponent *global_mqtt_client;
}}
