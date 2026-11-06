#include "rflink_native_safety.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
int main(){
  using namespace rflink;
  assert(native_ota_identity_accepted("rflink_universal",32,"0.7.0-n16r8-s14",32));
  assert(native_ota_identity_accepted("rflink_universal",32,"0.8.1-n16r8-p2",32));
  assert(!native_ota_identity_accepted("other_project",32,"0.7.0-n16r8-s14",32));
  assert(!native_ota_identity_accepted("rflink_universal",32,"0.7.0-n8r8-s14",32));
  assert(!native_ota_identity_accepted("rflink_universal",32,"0.7.0-stage13",32));
  char no_nul[32];std::memset(no_nul,'x',sizeof(no_nul));
  assert(!native_ota_identity_accepted(no_nul,sizeof(no_nul),"0.7.0-n16r8-s14",32));
  assert(!native_ota_identity_accepted("rflink_universal",32,no_nul,sizeof(no_nul)));
  BootHealth ok{true,true,true,300,2,3};
  assert(passes_boot_health(ok));
  ok.rx_cycles_during_window=0;assert(!passes_boot_health(ok));
  ok.rx_cycles_during_window=101;ok.active_receivers=0;assert(!passes_boot_health(ok));
  ok.active_receivers=1;ok.responsive_radios=0;assert(!passes_boot_health(ok));
  ok.responsive_radios=1;ok.web_started=false;assert(!passes_boot_health(ok));
  std::cout<<"PASS native OTA identity and live boot health 12 cases\n";
}
