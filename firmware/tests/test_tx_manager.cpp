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
  PhysicalTxResult transmit_rmt(RadioId radio,const RfProfile& p,const EncodedTxFrame& frame) override {
    assert(radio==RadioId::rf43342 || radio==RadioId::rf43392);
    assert(p.tx_transport==TxTransport::gdo_rmt && frame.pulse_count==2);
    events.push_back(7);return output;
  }
  PhysicalTxResult transmit_packet(RadioId radio,const RfProfile& p,const EncodedTxFrame& frame) override {
    assert(radio==RadioId::rf86830);assert(p.id==RfProfileId::packet_868_bench);
    assert(frame.length==16);events.push_back(8);return output;
  }
};
int main(){
  const uint8_t wire[4]={1,2,3,4};
  EncodedTxFrame tx{wire,sizeof(wire),1,0};
  Mock driver;RadioManager manager(driver);
  std::vector<TxTrace> trace;
  manager.set_tx_trace_observer([](void* ctx,const TxTrace& item){
    static_cast<std::vector<TxTrace>*>(ctx)->push_back(item);
  },&trace);
  // Eligibility checks never send, schedule a transmission or create
  // a misleading "transmitted" event.
  assert(manager.request_transmit(RadioId::rf86830,RfProfileId::ramses_868_packet)==ManagerError::no_hardware);
  assert(trace.empty());
  assert(manager.transmit_encoded(RadioId::rf86830,RfProfileId::ramses_868_packet,tx)==ManagerError::no_hardware);
  assert(trace.size()==1);
  assert(trace.back().phase==TxTracePhase::rejected);
  assert(!trace.back().driver_attempted);
  assert(trace.back().stage==TxFailureStage::none);
  manager.set_presence(RadioId::rf86830,true,true);
  assert(manager.transmit_encoded(RadioId::rf86830,RfProfileId::itho_868_packet,tx)==ManagerError::unsupported_transport);
  assert(manager.transmit_encoded(RadioId::rf86830,RfProfileId::ramses_868_packet,{wire,3,1,0})==ManagerError::invalid_payload);
  assert(manager.start_receive(RadioId::rf86830,RfProfileId::itho_868_packet)==ManagerError::none);
  driver.events.clear();
  assert(manager.transmit_encoded(RadioId::rf86830,RfProfileId::ramses_868_packet,tx)==ManagerError::none);
  assert((driver.events==std::vector<int>{2,3,1}));
  assert(trace.back().phase==TxTracePhase::finished);
  assert(trace.back().driver_attempted && trace.back().driver_accepted);
  assert(trace.back().was_receiving && trace.back().rx_restored);
  assert(trace.back().transport==TxTransport::gdo_uart);
  assert(trace.back().error==ManagerError::none);
  assert(trace[trace.size()-2].phase==TxTracePhase::started);
  assert(trace[trace.size()-2].correlation_id==trace.back().correlation_id);
  assert(manager.status(RadioId::rf86830).online());
  assert(manager.status(RadioId::rf86830).operation==RadioOperation::receiving);
  assert(manager.status(RadioId::rf86830).profile==RfProfileId::itho_868_packet);
  driver.events.clear();driver.output={false,true,TxFailureStage::uart_drain};
  assert(manager.transmit_encoded(RadioId::rf86830,RfProfileId::ramses_868_packet,tx)==ManagerError::tx_failure);
  assert((driver.events==std::vector<int>{2,3,1}));
  assert(manager.last_tx_stage(RadioId::rf86830)==TxFailureStage::uart_drain);
  assert(trace.back().stage==TxFailureStage::uart_drain);
  assert(trace.back().driver_attempted && !trace.back().driver_accepted);
  assert(trace.back().rx_restored && trace.back().error==ManagerError::tx_failure);
  assert(manager.status(RadioId::rf86830).online());
  driver.output={false,false,TxFailureStage::restore};
  assert(manager.transmit_encoded(RadioId::rf86830,RfProfileId::ramses_868_packet,tx)==ManagerError::restore_failure);
  assert(manager.status(RadioId::rf86830).operation==RadioOperation::fault);
  assert(trace.back().error==ManagerError::restore_failure);
  assert(trace.back().driver_attempted && !trace.back().hardware_restored);
  assert(!trace.back().rx_restored);
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
  assert(trace.back().error==ManagerError::none && !trace.back().was_receiving);
  assert(trace.back().rx_restored); // No RX restoration was needed.
  // No accidental 433 MHz GDO-RMT fallback to UART.
  manager.set_presence(RadioId::rf43342,true,true);
  assert(manager.transmit_encoded(RadioId::rf43342,RfProfileId::ook_43342_pulses,tx)==ManagerError::invalid_payload);
  assert(manager.transmit_encoded(RadioId::rf43342,RfProfileId::ramses_868_packet,tx)==ManagerError::incompatible_profile);
  // Generic trace also covers both OOK modules (RMT) and 868 FIFO.
  TxPulsePair pulses[]={{250,250},{400,400}};
  EncodedTxFrame rmt_frame{nullptr,0,1,0,pulses,2};
  for(auto radio:{RadioId::rf43342,RadioId::rf43392}){
    manager.set_presence(radio,true,true);
    assert(manager.start_receive(radio,
      radio==RadioId::rf43342?RfProfileId::ook_43342_pulses:
                               RfProfileId::ook_43392_pulses)==ManagerError::none);
    assert(manager.transmit_encoded(radio,
      radio==RadioId::rf43342?RfProfileId::ook_43342_pulses:
                               RfProfileId::ook_43392_pulses,rmt_frame)==ManagerError::none);
    assert(trace.back().radio==radio);
    assert(trace.back().units==2);
    assert(trace.back().transport==TxTransport::gdo_rmt);
    assert(trace.back().driver_attempted && trace.back().rx_restored);
  }
  uint8_t packet[16]={};
  manager.set_presence(RadioId::rf86830,true,true);
  assert(manager.start_receive(RadioId::rf86830,RfProfileId::itho_868_packet)==ManagerError::none);
  assert(manager.transmit_encoded(RadioId::rf86830,RfProfileId::packet_868_bench,
    {packet,sizeof(packet),1,0})==ManagerError::none);
  assert(trace.back().transport==TxTransport::packet_fifo);
  assert(trace.back().units==16 && trace.back().rx_restored);
  // An RX pause failure is logged *without* claiming the driver was called.
  driver.stop_ok=false;
  assert(manager.transmit_encoded(RadioId::rf86830,RfProfileId::packet_868_bench,
    {packet,sizeof(packet),1,0})==ManagerError::restore_failure);
  assert(trace.back().stage==TxFailureStage::rx_pause);
  assert(!trace.back().driver_attempted && !trace.back().rx_restored);
  assert(trace.back().error==ManagerError::restore_failure);
  // All TX attempts have distinct IDs; each start + finish pair shares one.
  for(size_t i=1;i<trace.size();++i){
    assert(trace[i].correlation_id>=trace[i-1].correlation_id);
    if(trace[i].correlation_id==trace[i-1].correlation_id)
      assert(trace[i-1].phase==TxTracePhase::started &&
             trace[i].phase==TxTracePhase::finished);
  }
  return 0;
}
