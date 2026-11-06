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
  assert(m.request_transmit(RadioId::rf86830,RfProfileId::itho_868_packet)==ManagerError::busy);
  assert(m.stop_receive(RadioId::rf86830)==ManagerError::none);
  assert(m.status(RadioId::rf86830).operation==RadioOperation::idle);
  assert(t.starts==1 && t.stops==1);
  t.succeed=false;
  assert(m.start_receive(RadioId::rf86830,RfProfileId::itho_868_packet)==ManagerError::driver_failure);
  assert(m.status(RadioId::rf86830).operation==RadioOperation::fault);
  assert(!m.status(RadioId::rf86830).online());
  m.set_presence(RadioId::rf86830,true,true);
  assert(m.status(RadioId::rf86830).online());
  return 0;
}
