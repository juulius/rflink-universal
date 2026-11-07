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
  // Real ESP-IDF default 100 Hz: a 5 ms pdMS_TO_TICKS truncates to zero.
  // CPU0 idle has to execute; verify the RX worker blocks for >= 1 tick.
  static_assert(rx_poll_delay_ticks(100)==1,"100 Hz RX worker starved IDLE0");
  static_assert(rx_poll_delay_ticks(1000)==5,"1000 Hz RX polling changed");
  static_assert(rx_poll_delay_ticks(250)==2,"round 5 ms up at 250 Hz");
  static_assert(rx_poll_delay_ticks(0)==1,"invalid rate must not busy-spin");
  assert(rx_poll_delay_ticks(100)==1);
  assert(rx_poll_delay_ticks(1000)==5);
  assert(!rx_recovery_due(4999,0));
  assert(rx_recovery_due(5000,0));
  assert(!rx_recovery_due(1,0));
  // Compare unsigned elapsed uptime correctly across the 49-day wrap.
  assert(rx_recovery_due(100,0xffffffffu-4900u));
  assert(!rx_recovery_due(100,0xffffffffu-1000u));
  BootHealth ok{true,true,true,300,2,3};
  assert(passes_boot_health(ok));
  ok.rx_cycles_during_window=0;assert(!passes_boot_health(ok));
  ok.rx_cycles_during_window=101;ok.active_receivers=0;assert(!passes_boot_health(ok));
  ok.active_receivers=1;ok.responsive_radios=0;assert(!passes_boot_health(ok));
  ok.responsive_radios=1;ok.web_started=false;assert(!passes_boot_health(ok));
  std::cout<<"PASS native OTA identity and live boot health 12 cases\n";
}
