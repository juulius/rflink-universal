#pragma once
// Presentation recommendations only. A protocol hit does NOT establish manufacturer/model.
// No profile recommendation is allowed to grant TX, pairing or Home Assistant consent.
#include <algorithm>
#include <string>
#include <vector>
#include "profile_catalog.h"

namespace esphome { namespace rflink_universal { namespace profile_suggestions {
struct Suggestion {
  std::string id;
  int score{0};
  bool action_evidence{false};
  bool manual_confirmation{true};
};
inline std::vector<Suggestion> rank(const std::string &protocol, const std::string &device_type,
                                    const std::vector<std::string> &observed_actions) {
  std::vector<Suggestion> result;
  size_t count = 0;
  const auto *items = profiles::all(count);
  for (size_t i = 0; i < count; ++i) {
    const auto &p = items[i];
    if (protocol != p.protocol) continue;
    if (std::strcmp(p.suggest_policy, "manual_select") == 0) continue;
    int score=100;
    if (!device_type.empty() && device_type == p.device_type) score+=15;
    bool action_match = false;
    const std::string expected=std::string("|")+p.expected_actions+"|";
    for (const auto &a : observed_actions)
      if (!a.empty() && expected.find("|"+a+"|")!=std::string::npos) { score+=4; action_match=true; }
    // Hard cap to avoid spuriously claiming certainty from repeated action types.
    score=std::min(score, 127);
    result.push_back({p.id,score,action_match,true});
  }
  std::stable_sort(result.begin(),result.end(),[](const Suggestion &a,const Suggestion &b){
    return a.score>b.score || (a.score==b.score && a.id<b.id);
  });
  return result;
}
// Sufficient to *suggest* a profile, never to assign it silently.
inline std::string unique_suggestion(const std::vector<Suggestion> &v) {
  return v.size()==1 ? v[0].id : std::string();
}
}}}
