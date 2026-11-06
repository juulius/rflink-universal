#include <cassert>
#include <vector>
#include "rflink_radio_manager.hpp"
using namespace rflink;
struct Mock : RadioTransport {
  std::vector<int> events;
  bool fail_restart = false;
  PhysicalTxResult tx_result{true,true,TxFailureStage::none};
  bool start_packet_rx(RadioId, const RfProfile&) override { events.push_back(11);return !fail_restart; }
  bool stop_packet_rx(RadioId) override { events.push_back(12);return true; }
  bool start_pulse_rx(RadioId, const RfProfile&) override { events.push_back(21);return !fail_restart; }
  bool stop_pulse_rx(RadioId) override { events.push_back(22);return true; }
  PhysicalTxResult transmit_rmt(RadioId, const RfProfile& p, const EncodedTxFrame& f) override {
    assert(p.tx_transport==TxTransport::gdo_rmt && f.pulse_count==2 && f.wire==nullptr);
    events.push_back(23);return tx_result;
  }
  PhysicalTxResult transmit_packet(RadioId, const RfProfile& p, const EncodedTxFrame& f) override {
    assert(p.tx_transport==TxTransport::packet_fifo && f.length==16 && f.pulses==nullptr);
    events.push_back(13);return tx_result;
  }
};
int main(){
  Mock d;RadioManager manager(d);
  manager.set_presence(RadioId::rf43342,true,true);
  manager.set_presence(RadioId::rf86830,true,true);
  TxPulsePair pulses[2]={{360,740},{710,380}};
  EncodedTxFrame pulse_frame{nullptr,0,2,9,pulses,2};
  assert(manager.start_receive(RadioId::rf43342,RfProfileId::ook_43342_pulses)==ManagerError::none);
  d.events.clear();
  assert(manager.transmit_encoded(RadioId::rf43342,RfProfileId::ook_43342_pulses,pulse_frame)==ManagerError::none);
  assert((d.events==std::vector<int>{22,23,21}));
  assert(manager.status(RadioId::rf43342).operation==RadioOperation::receiving);
  assert(manager.status(RadioId::rf43342).online());
  assert(manager.transmit_encoded(RadioId::rf43342,RfProfileId::ook_43342_pulses,{nullptr,0,1,0,pulses,0})==ManagerError::invalid_payload);
  TxPulsePair bad_pulses[1]={{0,500}};
  assert(manager.transmit_encoded(RadioId::rf43342,RfProfileId::ook_43342_pulses,{nullptr,0,1,0,bad_pulses,1})==ManagerError::invalid_payload);
  assert(manager.transmit_encoded(RadioId::rf43342,RfProfileId::ook_43392_pulses,pulse_frame)==ManagerError::incompatible_profile);
  d.tx_result={false,true,TxFailureStage::rmt_wait};d.events.clear();
  assert(manager.transmit_encoded(RadioId::rf43342,RfProfileId::ook_43342_pulses,pulse_frame)==ManagerError::tx_failure);
  assert((d.events==std::vector<int>{22,23,21}));
  assert(manager.last_tx_stage(RadioId::rf43342)==TxFailureStage::rmt_wait);
  assert(manager.status(RadioId::rf43342).online());
  d.tx_result={true,true,TxFailureStage::none}; d.fail_restart=true;
  assert(manager.transmit_encoded(RadioId::rf43342,RfProfileId::ook_43342_pulses,pulse_frame)==ManagerError::restore_failure);
  assert(manager.status(RadioId::rf43342).operation==RadioOperation::fault);
  manager.set_presence(RadioId::rf43342,true,true);d.fail_restart=false;
  uint8_t wire[16]={0};
  EncodedTxFrame packet_frame{wire,16,1,0};
  assert(manager.transmit_encoded(RadioId::rf86830,RfProfileId::packet_868_bench,packet_frame)==ManagerError::none);
  assert(d.events.back()==13);
  assert(manager.transmit_encoded(RadioId::rf86830,RfProfileId::packet_868_bench,{wire,15,1,0})==ManagerError::invalid_payload);
  assert(manager.transmit_encoded(RadioId::rf86830,RfProfileId::packet_868_bench,{nullptr,16,1,0})==ManagerError::invalid_payload);
  assert(manager.start_receive(RadioId::rf86830,RfProfileId::itho_868_packet)==ManagerError::none);
  d.events.clear();
  assert(manager.transmit_encoded(RadioId::rf86830,RfProfileId::packet_868_bench,packet_frame)==ManagerError::none);
  assert((d.events==std::vector<int>{12,13,11}));
  assert(manager.status(RadioId::rf86830).profile==RfProfileId::itho_868_packet);
  d.tx_result={false,false,TxFailureStage::restore};
  assert(manager.transmit_encoded(RadioId::rf86830,RfProfileId::packet_868_bench,packet_frame)==ManagerError::restore_failure);
  assert(!manager.status(RadioId::rf86830).online());
}
