#include <cassert>
#include <iostream>
#include "device_store.h"
#include "profile_catalog.h"
#include "profile_suggestions.h"
using namespace esphome::rflink_universal;
int main(){
  const auto* orcon=profiles::find("orcon_mvs15r_15rf");
  assert(orcon && std::string(orcon->protocol)=="RamsesII");
  assert(std::string(orcon->controls)=="away|auto|low|medium|high|timer_15|timer_30|timer_60");
  const auto suggestions=profile_suggestions::rank("RamsesII","ventilation",{"status"});
  assert(suggestions.size()==1 && suggestions[0].id=="orcon_15rf_general" && suggestions[0].manual_confirmation);
  assert(profile_suggestions::rank("not_real","",{}).empty());
  DeviceStore store;
  DecodeResult result; result.valid=true; result.protocol="RamsesII";
  result.device_type="ventilation"; result.identity_key="test:29:204810"; result.action="status";
  store.observe(result,100,2);
  assert(store.candidates().size()==1);
  assert(store.dismiss_candidate("test:29:204810"));
  assert(store.candidates().empty() && !store.dismiss_candidate("test:29:204810"));
  store.observe(result,150,2);
  store.observe(result,200,2);
  store.observe(result,250,2);
  auto *d=store.confirm_candidate("test:29:204810","Fan");
  assert(d && d->uid=="dev_00001");
  assert(!store.dismiss_candidate("test:29:204810")); // saved devices never deleted as proposals
  std::cout<<"PROFILEFLOW1: source-backed controls, honest profile suggestions, candidate discard, saved-device isolation PASS\n";
}
