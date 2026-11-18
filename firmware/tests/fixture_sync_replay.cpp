// Only synthetic, locally generated signals: NO captures or actual RF IDs.
// Prints one named, MSB-packed test bitstream per line for offline tests.
#include "rflink_itho_codec.hpp"
#include "rflink_ramses_codec.hpp"
#include "rflink_ramses_tx.hpp"
#include <array>
#include <cassert>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <vector>

static void emit(const char* name,const std::vector<uint8_t>& data,size_t count) {
  std::cout<<name<<" "<<count<<" ";
  for(uint8_t b:data)
    std::cout<<std::uppercase<<std::hex<<std::setw(2)<<std::setfill('0')<<unsigned(b);
  std::cout<<std::dec<<"\n";
}

int main(){
  const uint32_t controller=(37u<<18)|0x1234u;
  const uint32_t fan=(32u<<18)|0x5678u;
  rflink_ramses_ii::Frame frame;
  assert(rflink_ramses_ii::tx::buildCommand(
      {controller,fan},rflink_ramses_ii::tx::Action::AUTO,frame));
  std::vector<uint8_t> ramses;
  size_t count=0;
  assert(rflink_ramses_ii::tx::encodeOnAir(frame,2,ramses,count));
  emit("ramses",ramses,count);

  const auto logical=rflink::itho::build_speed_payload(
      0x16,{{0x11,0x22,0x33}},1,rflink::itho::SpeedCommand::LOW);
  const auto itho=rflink::itho::encode_payload(logical,0xAC);
  assert(itho.size()==63);
  emit("itho",itho,itho.size()*8);
  return 0;
}
