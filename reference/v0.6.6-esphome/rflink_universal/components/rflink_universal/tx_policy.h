#pragma once
#include <algorithm>
#include <cstdint>
#include <string>
#include "device_store.h"
#include "capability_catalog.h"

namespace esphome { namespace rflink_universal { namespace tx {
struct NexaCommand { uint32_t device{0}; uint8_t channel{0}, group{0}, level{0}; bool state{false}; };
inline bool decimal(const std::string &s, unsigned max, uint8_t &out) {
  if (s.empty() || s.size() > 3) return false;
  unsigned value=0;
  for (char ch : s) { if(ch<'0'||ch>'9') return false; value=value*10+unsigned(ch-'0'); }
  if (value>max) return false;
  out=static_cast<uint8_t>(value); return true;
}
inline const std::string *field(const DeviceRecord &d, const char *key) {
  for (const auto &f : d.fields) if(f.key==key) return &f.value;
  return nullptr;
}
inline const std::string *state(const DeviceRecord &d, const char *key) {
  for (const auto &f : d.tx_state) if(f.key==key) return &f.value;
  return nullptr;
}
inline bool unsigned_decimal(const std::string &s, uint32_t max, uint32_t &out) {
  if (s.empty() || s.size() > 10) return false;
  uint64_t value=0;
  for (char ch:s) { if(ch<'0'||ch>'9') return false; value=value*10+uint64_t(ch-'0'); if(value>max)return false; }
  out=static_cast<uint32_t>(value); return true;
}
inline bool nexa(const DeviceRecord &d, const std::string &action, NexaCommand &out) {
  // Manifest must explicitly register a supported, reviewed encoder/action.
  if (!capabilities::tx_allowed(d.protocol.c_str(), action.c_str()) ||
      d.protocol!="Nexa" || !d.transmit_supported ||
      d.identity_confidence!=IdentityConfidence::HIGH || d.rf_identities.size()!=1 ||
      (action!="on" && action!="off") ||
      std::find(d.actions.begin(),d.actions.end(),action)==d.actions.end()) return false;
  const auto &identity=d.rf_identities.front();
  if(identity.compare(0,5,"nexa:")!=0 || identity.size()<6 || identity.size()>13) return false;
  uint64_t id=0;
  for(size_t i=5;i<identity.size();i++) {
    const char ch=identity[i];
    const int digit=ch>='0'&&ch<='9'?ch-'0':ch>='A'&&ch<='F'?ch-'A'+10:-1;
    if(digit<0) return false;
    id=(id<<4)|static_cast<unsigned>(digit);
  }
  if(id>0xFFFFFFFFULL) return false;
  // The user confirms the current RX details when enabling TX. Once enabled,
  // always send with that immutable snapshot, never with a later RX frame's
  // channel/group/level (multi-channel remotes can change these on reception).
  const bool locked=d.transmit_enabled;
  const auto *rf=field(d,locked?"tx_lock_radio":"tx_radio");
  const auto *ch=field(d,locked?"tx_lock_channel":"channel");
  const auto *grp=field(d,locked?"tx_lock_group":"group");
  const auto *lvl=field(d,locked?"tx_lock_level":"level");
  uint8_t radio=255, channel=0, level=0;
  if(!rf || !decimal(*rf,2,radio) || radio!=1 || !ch || !decimal(*ch,15,channel) ||
     !grp || (*grp!="true" && *grp!="false") || !lvl || !decimal(*lvl,15,level)) return false;
  out.device=static_cast<uint32_t>(id);out.channel=channel;out.group=*grp=="true"?1:0;
  out.level=level;out.state=action=="on";
  return true;
}
inline bool eligible(const DeviceRecord &d) {
  NexaCommand unused;
  return nexa(d,"on",unused) || nexa(d,"off",unused);
}
}}} // namespaces
