// Experimental RAMSES-II commands only become eligible with ACK-backed virtual
// controller+fan state, reviewed MVS profile and explicit TX-capable device.
#include <cassert>
#include <cstdio>
#include "tx_engine.h"
#include "tx_codec.h"
using namespace esphome::rflink_universal;
int main(){
 tx_engine::Plan plan;const tx_engine::Encoder *encoder=nullptr;
 DeviceRecord d;d.uid="dev_orcon";d.name="fan";d.protocol="RamsesII";
 d.profile_id="orcon_mvs15r_15rf";d.transmit_supported=true;
 d.actions={"mode_command"};
 assert(tx_engine::resolve("RamsesII")!=nullptr);
 for(const std::string action:{"away","auto","low","medium","high","timer_15","timer_30","timer_60"}){
   assert(!tx_engine::prepare(d,action,plan,encoder)); // unpaired
 }
 const uint32_t controller=(37u<<18)|0x1CAFEu,fan=0x123456u;
 d.tx_state={{"ramses_controller",std::to_string(controller),""},{"ramses_fan",std::to_string(fan),""},
             {"ramses_paired","1",""}};
 for(const std::string action:{"away","auto","low","medium","high","timer_15","timer_30","timer_60"}){
   assert(tx_engine::prepare(d,action,plan,encoder));
   assert(plan.radio==2&&plan.transport==TxTransport::GDO_SERIAL&&plan.repeats==1);
   std::vector<uint8_t> serial;
   assert(encoder->encode_serial(serial,plan) && serial.size()>10&&serial[0]==0xff);
   rflink_ramses_ii::Frame decoded;
   assert(rflink_ramses_ii::decodeWire(serial.data()+5,serial.size()-5,decoded));
   assert(decoded.has_addr[0]&&decoded.has_addr[1] && decoded.addr[0]==controller && decoded.addr[1]==fan);
   assert(decoded.command==(action.find("timer_")==0?0x22F3:0x22F1));
 }
 assert(!tx_engine::prepare(d,"prog",plan,encoder));
 d.tx_state[2].value="0";assert(!tx_engine::prepare(d,"auto",plan,encoder));
 d.tx_state[2].value="1";d.tx_state[1].value=std::to_string(controller);assert(!tx_engine::prepare(d,"auto",plan,encoder));
 d.tx_state[1].value=std::to_string(fan);d.profile_id="orcon_15rf_general";assert(!tx_engine::prepare(d,"auto",plan,encoder));
 puts("Experimental Orcon native encoder: eight commands, UART wire, no unpaired/control override PASS");
}
