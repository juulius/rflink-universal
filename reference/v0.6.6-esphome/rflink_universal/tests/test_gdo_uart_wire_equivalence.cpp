#include "../decoders/ramses_ii/tx_codec.h"
#include <cassert>
#include <iostream>
#include <vector>
// A real 8N1 UART clocks start=0, then LSB-first data, then stop=1.
// Verify that this is precisely the physical bitstream specified by RAMSES codec.
int main(){
 using namespace rflink_ramses_ii;
 tx::Ids ids{(37U<<18)|123456U,(32U<<18)|82155U};
 for(auto cmd : {tx::Action::AWAY,tx::Action::LOW,tx::Action::MEDIUM,
                 tx::Action::HIGH,tx::Action::AUTO,tx::Action::TIMER_15,
                 tx::Action::TIMER_30,tx::Action::TIMER_60}){
  Frame frame;
  assert(tx::buildCommand(ids,cmd,frame));
  std::vector<uint8_t> bytes,packed;
  size_t nbits=0;
  assert(tx::encodeWire(frame,bytes));
  assert(tx::encodeOnAir(frame,0,packed,nbits));
  assert(nbits==bytes.size()*10);
  for(size_t i=0;i<nbits;++i){
   const unsigned bit=i%10;
   const bool expected=bit==0?false:(bit==9?true:(bytes[i/10]>>(bit-1))&1);
   const bool got=(packed[i/8]>>(7-(i%8)))&1;
   assert(expected==got);
  }
 }
 std::cout<<"8 RAMSES-II commands: UART 8N1 waveform exactly matches codec: PASS\n";
}
