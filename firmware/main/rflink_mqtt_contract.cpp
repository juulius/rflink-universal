#include "rflink_mqtt.hpp"
#include <cstring>
#include <cstdio>
namespace rflink {
bool mqtt_valid_host(const char* p) {
  if(!p) return false;
  const size_t n=strnlen(p,96);
  if(n==0 || n>=96 || p[0]=='.' || p[0]=='-' || p[n-1]=='.' || p[n-1]=='-') return false;
  for(size_t i=0;i<n;i++) {
    const char c=p[i];
    const bool ok=(c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='.'||c=='-';
    if(!ok) return false;
  }
  return true;
}
bool mqtt_valid_settings(const MqttSettings& s) {
  if(s.schema!=1 || !mqtt_valid_host(s.host) || s.port==0 || s.tls!=0 || s.enabled!=1) return false;
  if(strnlen(s.username,sizeof(s.username))>=sizeof(s.username) ||
     strnlen(s.password,sizeof(s.password))>=sizeof(s.password)) return false;
  return true;
}
std::string mqtt_hub_id(const uint8_t mac[6]) {
  char out[32];
  snprintf(out,sizeof(out),"rflink_%02x%02x%02x%02x%02x%02x",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  return out;
}
} // namespace rflink
