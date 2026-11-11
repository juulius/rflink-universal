#pragma once
// Build an immutable reception plan from decoder requirements. No direct
// register writes, no speculative time sharing. Equal routes share one capture.
#include "radio_contract_v2.h"
#include <vector>
#include <cstring>

namespace esphome { namespace rflink_universal {
struct RadioCaptureGroup {
  uint8_t channel{0};
  InputKind kind{InputKind::RAW};
  const char *profile{nullptr};
  const char *method{nullptr};
  uint16_t subscribers{0};
};
class RadioProfilePlanV2 {
 public:
  RadioProfilePlanV2(const RadioInputSpec *specs,size_t count) {
    for(size_t i=0;i<count;++i) {
      const auto &s=specs[i];
      bool found=false;
      for(auto &group:groups_) {
        if(group.channel==s.channel && group.kind==s.kind &&
           equal(group.profile,s.profile) && equal(group.method,s.capture_method)) {
          ++group.subscribers;found=true;break;
        }
      }
      if(!found) groups_.push_back({s.channel,s.kind,s.profile,s.capture_method,1});
    }
  }
  const std::vector<RadioCaptureGroup> &groups() const {return groups_;}
  // Multiple groups on the same chip cannot all receive continuously. Caller
  // must explicitly approve a switching schedule; never start one implicitly.
  bool can_listen_without_switch(uint8_t channel) const {
    size_t n=0;
    for(const auto &g:groups_) if(g.channel==channel) ++n;
    return n<=1;
  }
  size_t groups_on(uint8_t channel) const {
    size_t n=0;for(const auto &g:groups_)if(g.channel==channel)++n;return n;
  }
 private:
  static bool equal(const char *a,const char *b) {return a&&b&&std::strcmp(a,b)==0;}
  std::vector<RadioCaptureGroup> groups_;
};
}}
