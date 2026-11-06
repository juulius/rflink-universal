#include <cassert>
#include <vector>
#include "rflink_radio_manager.hpp"
using namespace rflink;
struct Mock : RadioTransport {
  std::vector<int> events;
  bool start_ok = true, stop_ok = true;
  PhysicalTxResult output{true,true,TxFailureStage::none};
  bool start_packet_rx(RadioId, const RfProfile&) override {events.push_back(1);return start_ok;}
  bool stop_packet_rx(RadioId) override {events.push_back(2);return stop_ok;}
  bool start_pulse_rx(RadioId,const RfProfile&) override {events.push_back(4);return start_ok;}
  bool stop_pulse_rx(RadioId) override {events.push_back(5);return stop_ok;}
  PhysicalTxResult transmit_uart(RadioId radio,const RfProfile& p,const EncodedTxFrame& tx) override {
    assert(radio==RadioId::rf86830);assert(p.id==RfProfileId::ramses_868_packet);
    assert(tx.length==4);events.push_back(3);return output;
  }
};
int main(){
  const uint8_t wire[4]={1,2,3,4};
  EncodedTxFrame tx{wire,sizeof(wire),1,0};
  Mock driver;RadioManager manager(driver);
  assert(manager.transmit_encoded(RadioId::rf86830,RfProfileId::ramses_868_packet,tx)==ManagerError::no_hardware);
  manager.set_presence(RadioId::rf86830,true,true);
  assert(manager.transmit_encoded(RadioId::rf86830,RfProfileId::itho_868_packet,tx)==ManagerError::unsupported_transport);
  assert(manager.transmit_encoded(RadioId::rf86830,RfProfileId::ramses_868_packet,{wire,3,1,0})==ManagerError::invalid_payload);
  assert(manager.start_receive(RadioId::rf86830,RfProfileId::itho_868_packet)==ManagerError::none);
  driver.events.clear();
  assert(manager.transmit_encoded(RadioId::rf86830,RfProfileId::ramses_868_packet,tx)==ManagerError::none);
  assert((driver.events==std::vector<int>{2,3,1}));
  assert(manager.status(RadioId::rf86830).online());
  assert(manager.status(RadioId::rf86830).operation==RadioOperation::receiving);
  assert(manager.status(RadioId::rf86830).profile==RfProfileId::itho_868_packet);
  driver.events.clear();driver.output={false,true,TxFailureStage::uart_drain};
  assert(manager.transmit_encoded(RadioId::rf86830,RfProfileId::ramses_868_packet,tx)==ManagerError::tx_failure);
  assert((driver.events==std::vector<int>{2,3,1}));
  assert(manager.last_tx_stage(RadioId::rf86830)==TxFailureStage::uart_drain);
  assert(manager.status(RadioId::rf86830).online());
  driver.output={false,false,TxFailureStage::restore};
  assert(manager.transmit_encoded(RadioId::rf86830,RfProfileId::ramses_868_packet,tx)==ManagerError::restore_failure);
  assert(manager.status(RadioId::rf86830).operation==RadioOperation::fault);
  assert(!manager.status(RadioId::rf86830).online());
  manager.set_presence(RadioId::rf86830,true,true);
  driver.output={true,true,TxFailureStage::none};
  assert(manager.start_receive(RadioId::rf86830,RfProfileId::itho_868_packet)==ManagerError::none);
  driver.start_ok=false;
  assert(manager.transmit_encoded(RadioId::rf86830,RfProfileId::ramses_868_packet,tx)==ManagerError::restore_failure);
  assert(!manager.status(RadioId::rf86830).online());
  driver.start_ok=true;
  manager.set_presence(RadioId::rf86830,true,true);
  assert(manager.transmit_encoded(RadioId::rf86830,RfProfileId::ramses_868_packet,tx)==ManagerError::none);
  assert(manager.status(RadioId::rf86830).operation==RadioOperation::idle);
  // No accidental 433 MHz GDO-RMT fallback to UART.
  manager.set_presence(RadioId::rf43342,true,true);
  assert(manager.transmit_encoded(RadioId::rf43342,RfProfileId::ook_43342_pulses,tx)==ManagerError::invalid_payload);
  assert(manager.transmit_encoded(RadioId::rf43342,RfProfileId::ramses_868_packet,tx)==ManagerError::incompatible_profile);
  return 0;
}
