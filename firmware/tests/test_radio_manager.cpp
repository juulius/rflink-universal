#include <cassert>
#include "rflink_radio_manager.hpp"
using namespace rflink;
struct FakeTransport : RadioTransport {
  int starts=0,stops=0;bool succeed=true;
  bool start_packet_rx(RadioId,const RfProfile&) override {++starts;return succeed;}
  bool stop_packet_rx(RadioId) override {++stops;return succeed;}
  bool start_pulse_rx(RadioId,const RfProfile&) override {++starts;return succeed;}
  bool stop_pulse_rx(RadioId) override {++stops;return succeed;}
};
int main(){
  FakeTransport t;RadioManager m(t);
  assert(!m.status(RadioId::rf86830).online());
  assert(m.start_receive(RadioId::rf86830,RfProfileId::itho_868_packet)==ManagerError::no_hardware);
  m.set_presence(RadioId::rf86830,true,true);
  assert(m.status(RadioId::rf86830).online());
  assert(m.start_receive(RadioId::rf86830,RfProfileId::ramses_868_packet)==ManagerError::missing_profile);
  assert(m.start_receive(RadioId::rf86830,RfProfileId::ook_43392_pulses)==ManagerError::incompatible_profile);
  assert(m.request_transmit(RadioId::rf86830,RfProfileId::itho_868_packet)==ManagerError::unsupported_transport);
  assert(m.start_receive(RadioId::rf86830,RfProfileId::itho_868_packet)==ManagerError::none);
  assert(m.status(RadioId::rf86830).online());
  assert(m.status(RadioId::rf86830).operation==RadioOperation::receiving);
  m.set_presence(RadioId::rf86830,false,false); // must not override active state
  assert(m.status(RadioId::rf86830).online());
  assert(m.start_receive(RadioId::rf86830,RfProfileId::itho_868_packet)==ManagerError::busy);
  // Active RX is eligible for a *supported* TX transport; the shared RX
  // profile itself has no UART TX preset and therefore is not transmit-ready.
  assert(m.request_transmit(RadioId::rf86830,RfProfileId::itho_868_packet)==ManagerError::unsupported_transport);
  assert(m.request_transmit(RadioId::rf86830,RfProfileId::ramses_868_packet)==ManagerError::none);
  assert(m.status(RadioId::rf86830).operation==RadioOperation::receiving);
  assert(m.stop_receive(RadioId::rf86830)==ManagerError::none);
  assert(m.status(RadioId::rf86830).operation==RadioOperation::idle);
  assert(t.starts==1 && t.stops==1);
  t.succeed=false;
  assert(m.start_receive(RadioId::rf86830,RfProfileId::itho_868_packet)==ManagerError::driver_failure);
  assert(m.status(RadioId::rf86830).operation==RadioOperation::fault);
  assert(!m.status(RadioId::rf86830).online());
  m.set_presence(RadioId::rf86830,true,true);
  assert(m.status(RadioId::rf86830).online());
  // A read-only hardware health callback is only run during active RX.
  bool checked=false;
  auto sample=[](void* context,RadioId)->bool {
    *static_cast<bool*>(context)=true;
    return false; // Simulated unpowered radio.
  };
  bool healthy=true;
  assert(!m.sample_receiving_health(RadioId::rf86830,sample,&checked,&healthy));
  t.succeed=true;
  assert(m.start_receive(RadioId::rf86830,RfProfileId::itho_868_packet)==ManagerError::none);
  assert(m.sample_receiving_health(RadioId::rf86830,sample,&checked,&healthy));
  assert(checked && !healthy);
  // A missing module cannot acknowledge SIDLE. The manager must still
  // release the stale RX driver and fail closed without a global reboot.
  t.succeed=false;
  const int old_stops=t.stops;
  assert(m.mark_hardware_lost(RadioId::rf86830));
  assert(t.stops==old_stops+1);
  const auto lost=m.status(RadioId::rf86830);
  assert(!lost.present && !lost.spi_healthy && !lost.online());
  assert(lost.operation==RadioOperation::unavailable);
  assert(m.start_receive(RadioId::rf86830,RfProfileId::itho_868_packet)==ManagerError::no_hardware);
  // Recovery remains possible after a good read-only probe.
  m.set_presence(RadioId::rf86830,true,true);
  t.succeed=true;
  assert(m.start_receive(RadioId::rf86830,RfProfileId::itho_868_packet)==ManagerError::none);
  return 0;
}
