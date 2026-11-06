#include <cassert>
#include <vector>
#include "rflink_ramses_tx.hpp"
#include "rflink_ramses_codec.hpp"
int main(){
  namespace r=rflink_ramses_ii;
  const uint32_t controller=(37u<<18)|0x1234;
  const uint32_t fan=(13u<<18)|0x4567;
  r::Frame offer,cmd,decoded;
  assert(r::tx::buildBindOffer(controller,offer));
  assert(offer.command==0x1fc9 && offer.payload_length==18);
  std::vector<uint8_t> wire;
  assert(r::tx::encodeWire(offer,wire));
  assert(wire.size()>=4 && wire.size()<=512);
  assert(r::decodeWire(wire.data(),wire.size(),decoded));
  assert(decoded.command==0x1fc9 && decoded.addr[0]==controller);
  assert(r::tx::buildCommand({controller,fan},r::tx::Action::AUTO,cmd));
  assert(r::tx::encodeWire(cmd,wire));
  assert(r::decodeWire(wire.data(),wire.size(),decoded));
  assert(decoded.command==0x22f1 && decoded.payload[1]==4);
  assert(!r::tx::buildCommand({fan,fan},r::tx::Action::HIGH,cmd));
  // Pairing acknowledgement must be addressed to the virtual remote,
  // never accepted from unrelated device traffic.
  uint32_t fan_out=0;
  assert(!r::tx::isBindAcknowledgement(decoded,controller,fan_out));
  return 0;
}
