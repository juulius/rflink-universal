#include <cassert>
#include <cstdint>
#include <iostream>
#include "radio_mode_manager.h"
using esphome::rflink_universal::RadioModeManager;
struct Fake : RadioModeManager::Backend {
  bool pause_ok=true, start_ok=true, stop_ok=true, restore_ok=true, verify_ok=true;
  int pause=0,start=0,stop=0,restore=0,verify=0;
  bool pause_itho() override { ++pause; return pause_ok; }
  bool start_ramses_rx() override { ++start; return start_ok; }
  bool stop_ramses() override { ++stop; return stop_ok; }
  bool restore_itho() override { ++restore; return restore_ok; }
  bool verify_itho() override { ++verify; return verify_ok; }
};
int main() {
  RadioModeManager m;
  assert(m.itho_ready() && !m.hardware_adapter_present());
  assert(!m.request_ramses_rx(0,100)); // fail-closed until real backend installed
  assert(m.last_error()==RadioModeManager::Error::DISABLED);
  Fake hw;
  assert(m.arm(&hw) && !m.arm(&hw));
  assert(!m.request_ramses_rx(1,0));
  assert(!m.request_ramses_rx(1,RadioModeManager::MAX_LEASE_MS+1));
  assert(m.request_ramses_rx(100,1000));
  assert(!m.itho_ready()); // reservation itself excludes packet TX before SPI change
  assert(!m.request_ramses_rx(101,1000));
  assert(!m.disarm());
  m.poll(101);
  assert(m.ramses_listening() && hw.pause==1 && hw.start==1);
  m.poll(1099); assert(m.ramses_listening());
  m.poll(1100);
  assert(m.itho_ready() && hw.stop==1 && hw.restore==1 && hw.verify==1);
  assert(m.successful_restores()==1 && m.failed_restores()==0);
  assert(m.request_ramses_rx(2000,50));m.poll(2000);m.request_restore();m.poll(2010);
  assert(m.itho_ready() && m.successful_restores()==2);
  // Failure during acquisition must still attempt clean restore.
  hw.start_ok=false;
  assert(m.request_ramses_rx(3000,100));m.poll(3000);
  assert(m.itho_ready() && m.last_error()==RadioModeManager::Error::RAMSES_START_FAILED);
  hw.start_ok=true;
  // An unverified restore MUST stick in fault and deny further leases.
  hw.verify_ok=false;
  assert(m.request_ramses_rx(4000,100));m.poll(4000);m.poll(4100);
  assert(!m.itho_ready() && m.state()==RadioModeManager::State::FAULT);
  assert(!m.request_ramses_rx(4200,100));
  hw.verify_ok=true;
  assert(m.try_recover());m.poll(4200);
  assert(m.itho_ready());
  // Handle 32-bit millis rollover without early expiry.
  assert(m.request_ramses_rx(UINT32_MAX-50,100));
  m.poll(UINT32_MAX-50);m.poll(20);assert(m.ramses_listening());
  m.poll(49);assert(m.itho_ready());
  // Restoration failures are sticky even if a later probe returns good.
  hw.restore_ok=false;
  assert(m.request_ramses_rx(10000,100));m.poll(10000);m.poll(10100);
  assert(m.state()==RadioModeManager::State::FAULT);
  hw.restore_ok=true;assert(m.try_recover());m.poll(10101);assert(m.itho_ready());
  assert(m.disarm());assert(!m.request_ramses_rx(11111,100));
  std::cout<<"Radio mode manager: fail-closed arbitration, recovery, timeout, restoration, rollover PASS\n";
}
