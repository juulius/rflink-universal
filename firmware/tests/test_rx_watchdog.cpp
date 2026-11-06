#include <cassert>
#include "rflink_rx_watchdog.hpp"
using namespace rflink;

int main() {
  ManagedRadio s{};
  // Missing radio and driver failures must be re-probed, never blind-reset.
  assert(rx_watchdog_action(s,false)==RxWatchdogAction::probe_then_start);
  s.present=true;s.spi_healthy=true;s.operation=RadioOperation::idle;
  assert(rx_watchdog_action(s,false)==RxWatchdogAction::start);
  s.operation=RadioOperation::receiving;
  assert(rx_watchdog_action(s,true)==RxWatchdogAction::none);
  assert(rx_watchdog_action(s,false)==RxWatchdogAction::stop_then_restart);
  s.operation=RadioOperation::fault;
  assert(rx_watchdog_action(s,false)==RxWatchdogAction::probe_then_start);
  s.operation=RadioOperation::switching;
  assert(rx_watchdog_action(s,false)==RxWatchdogAction::none);
  s.operation=RadioOperation::transmitting;
  assert(rx_watchdog_action(s,false)==RxWatchdogAction::none);
  s.operation=RadioOperation::unavailable;
  assert(rx_watchdog_action(s,false)==RxWatchdogAction::probe_then_start);
  // Test the decision with a manager-driven RX->lost->recover sequence.
  struct Driver : RadioTransport {
    int starts=0,stops=0;
    bool start_packet_rx(RadioId,const RfProfile&) override {++starts;return true;}
    bool stop_packet_rx(RadioId) override {++stops;return true;}
    bool start_pulse_rx(RadioId,const RfProfile&) override {++starts;return true;}
    bool stop_pulse_rx(RadioId) override {++stops;return true;}
  } driver;
  RadioManager manager(driver);
  for(unsigned i=0;i<3;i++){
    auto id=static_cast<RadioId>(i);
    const auto profile = i==2?RfProfileId::itho_868_packet:
      i==0?RfProfileId::ook_43342_pulses:RfProfileId::ook_43392_pulses;
    assert(rx_watchdog_action(manager.status(id),false)==RxWatchdogAction::probe_then_start);
    manager.set_presence(id,true,true);
    assert(manager.start_receive(id,profile)==ManagerError::none);
    assert(rx_watchdog_action(manager.status(id),false)==RxWatchdogAction::stop_then_restart);
    assert(manager.stop_receive(id)==ManagerError::none);
    assert(manager.start_receive(id,profile)==ManagerError::none);
    assert(manager.status(id).operation==RadioOperation::receiving);
  }
  assert(driver.starts==6 && driver.stops==3);
}
