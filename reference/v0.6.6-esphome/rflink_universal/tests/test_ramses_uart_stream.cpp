#include "../components/rflink_universal/ramses_uart_stream.h"
#include "../decoders/ramses_ii/tx_codec.h"
#include <cassert>
#include <vector>
#include <cstdio>
using namespace esphome::rflink_universal;
using namespace rflink_ramses_ii;
int main() {
  Frame f;
  const uint32_t rem=(37u<<18)|123456u,fan=(32u<<18)|54321u;
  assert(tx::buildCommand({rem,fan},tx::Action::AUTO,f));
  std::vector<uint8_t> wire;
  assert(tx::encodeWire(f,wire));
  RamsesUartStream stream;
  Frame got;
  uint8_t noise[]={0x13,0x33,0x9A,0x00,0x33,0x55};
  assert(!stream.feed(noise,sizeof(noise),100,got));
  // Fragmented bytes across event-loop reads. Valid checksum must survive.
  for(size_t i=0;i<wire.size()-1;++i)
    assert(!stream.feed(wire.data()+i,1,102+uint32_t(i/5),got));
  assert(stream.feed(wire.data()+wire.size()-1,1,115,got));
  assert(got.command==0x22F1 && got.payload[1]==4);
  // Invalid checksum must never result in a frame.
  auto bad=wire;bad[bad.size()-5]^=0x40;
  for(size_t i=0;i<bad.size();++i)assert(!stream.feed(&bad[i],1,150,got));
  assert(stream.rejected()>0);
  stream.reset();
  // A partial frame exceeding timeout cannot join the next frame.
  assert(!stream.feed(wire.data(),12,200,got));
  assert(stream.feed(wire.data(),wire.size(),500,got));
  assert(got.command==0x22F1);
  // Several complete messages in one UART read must not be silently lost.
  std::vector<uint8_t> together;
  together.insert(together.end(),wire.begin(),wire.end());
  together.insert(together.end(),wire.begin(),wire.end());
  stream.reset();
  assert(stream.feed(together.data(),together.size(),800,got));
  assert(got.command==0x22F1);
  assert(stream.take_pending(got) && got.command==0x22F1);
  assert(!stream.take_pending(got));
  // Unframed noise is bounded (must not overflow the buffer).
  uint8_t garbage[220];for(size_t i=0;i<220;++i)garbage[i]=0xFF;
  for(unsigned i=0;i<100;++i)assert(!stream.feed(garbage,sizeof(garbage),1000+i,got));
  std::puts("UART stream deframing: PASS");
}
