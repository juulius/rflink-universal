#include "../components/rflink_universal/gdo_serial_tx_session.h"
#include <cassert>
#include <iostream>
#include <string>
using namespace esphome::rflink_universal;
struct Fake : GdoSerialTxOperations {
 bool allow=true,prep=true,send_ok=true,restore=true,verify=true;
 const char *failure_detail() const override { return "UART1 8N1/baud config: ESP_ERR_INVALID_ARG (0x102)"; }
 int acquired=0,preps=0,sent=0,restores=0,verified=0,waits=0;
 bool acquire() override {if (!allow)return false; ++acquired;return true;}
 bool prepare_tx() override {++preps;return prep;}
 bool transmit_frame(const uint8_t *p,size_t n) override { assert(p && n==5);++sent;return send_ok;}
 void between_frames(uint16_t gap) override {assert(gap==35);++waits;}
 bool restore_rx() override {++restores;return restore;}
 bool verify_rx() override {++verified;return verify;}
};
int main(){
 const uint8_t b[5]={0xFF,0x00,0x33,0x55,0x53};std::string err;
 {Fake o;GdoSerialTxSession s(o);assert(s.run(b,5,3,35,err));assert(o.sent==3 && o.waits==2 && o.restores==1 && o.verified==1);}
 {Fake o;GdoSerialTxSession s(o);assert(!s.run(b,5,0,35,err));assert(o.acquired==0 && o.restores==0);}
 {Fake o;o.allow=false;GdoSerialTxSession s(o);assert(!s.run(b,5,1,0,err));assert(o.restores==0 && s.ready());}
 {Fake o;o.prep=false;GdoSerialTxSession s(o);assert(!s.run(b,5,1,0,err));assert(o.sent==0 && o.restores==1 && o.verified==1 && s.ready());
   assert(err.find("UART1 8N1/baud config: ESP_ERR_INVALID_ARG")!=std::string::npos);}
 {Fake o;o.send_ok=false;GdoSerialTxSession s(o);assert(!s.run(b,5,3,35,err));assert(o.sent==1 && o.restores==1 && o.verified==1 && s.ready());
   assert(err.find("UART1 8N1/baud config: ESP_ERR_INVALID_ARG")!=std::string::npos);}
 {Fake o;o.restore=false;GdoSerialTxSession s(o);assert(!s.run(b,5,1,0,err));assert(s.fault() && o.verified==0);assert(!s.run(b,5,1,0,err) && o.acquired==1);assert(!s.clear_fault_after_external_rx_verification(false));assert(s.clear_fault_after_external_rx_verification(true));}
 {Fake o;o.verify=false;GdoSerialTxSession s(o);assert(!s.run(b,5,1,0,err));assert(s.fault() && o.verified==1);}
 std::cout<<"GDO serial TX: repeat cadence, fail-closed, restore and sticky fault PASS\n";
}
