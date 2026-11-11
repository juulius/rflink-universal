#include "../components/rflink_universal/serial_tx_adapter.h"
#include <cassert>
#include <iostream>
using namespace esphome::rflink_universal;
struct Mock: SerialTxAdapter {
 bool online{false},restored{false}; int writes{0};
 bool ready(uint8_t radio,const char *preset) const override {
   return online && radio==2 && std::string(preset)=="fsk_86830_ramses_gdo_serial_lab";
 }
 bool send(uint8_t radio,const char *preset,const uint8_t *data,size_t n,
           uint8_t times,uint16_t gap,std::string &error) override {
   if(!ready(radio,preset) || !data || n<4 || times==0 || gap>1000) {
    error="precondition failure";return false;
   }
   writes=times;restored=true;return true;
 }
};
int main(){
 assert(!serial_wire_sane({},1,0));
 assert(!serial_wire_sane(std::vector<uint8_t>(513),1,0));
 assert(!serial_wire_sane(std::vector<uint8_t>(4),0,0));
 assert(serial_wire_sane(std::vector<uint8_t>(4),3,40));
 Mock hw;assert(!hw.ready(2,"fsk_86830_ramses_gdo_serial_lab"));
 hw.online=true;std::vector<uint8_t> bytes{0xff,0,0x33,0x55};std::string err;
 assert(hw.send(2,"fsk_86830_ramses_gdo_serial_lab",bytes.data(),bytes.size(),3,40,err));
 assert(hw.writes==3 && hw.restored);
 assert(!hw.send(0,"fsk_86830_ramses_gdo_serial_lab",bytes.data(),bytes.size(),1,0,err));
 std::cout<<"Serial TX adapter contract and fail-closed gates: PASS\n";
}
