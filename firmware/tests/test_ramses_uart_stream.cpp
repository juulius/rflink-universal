// Pure C++ regression for the legacy v0.6.6 UART receive logic port.
// Synthetic IDs and commands only; never sends RF or touches the CC1101.
#include "rflink_ramses_uart_stream.hpp"
#include "rflink_ramses_tx.hpp"
#include <cassert>
#include <cstdio>
#include <vector>

using rflink_ramses_ii::Frame;
using rflink_ramses_ii::UartStreamFramer;

static std::vector<uint8_t> make_wire() {
  Frame msg;
  msg.type = Frame::INFORMATION;
  const uint32_t src = (37u << 18) | 0x1234u;
  const uint32_t dst = (32u << 18) | 0x5678u;
  msg.has_addr[0] = true;
  msg.has_addr[1] = true;
  msg.addr[0] = src;
  msg.addr[1] = dst;
  msg.command = 0x22F1;
  msg.payload_length = 3;
  msg.payload[0] = 0;
  msg.payload[1] = 4;
  msg.payload[2] = 4;
  std::vector<uint8_t> out;
  assert(rflink_ramses_ii::tx::encodeWire(msg, out));
  return out;
}

int main() {
  const auto wire = make_wire();
  UartStreamFramer stream;
  Frame got;
  const uint8_t noise[] = {0x13,0x33,0x9a,0x00,0x33,0x55};
  assert(!stream.feed(noise,sizeof(noise),100,got));

  // Chunked UART reads must preserve a correctly framed packet.
  for(size_t i=0; i+1<wire.size(); ++i)
    assert(!stream.feed(&wire[i],1,102+static_cast<uint32_t>(i/5),got));
  assert(stream.feed(&wire.back(),1,115,got));
  assert(got.command==0x22F1 && got.payload_length==3 && got.payload[1]==4);

  // Invalid checksum/Manchester must be rejected, never surfaced as valid.
  auto bad = wire;
  bad[bad.size()-5] ^= 0x40;
  const auto before_rejected=stream.rejected();
  assert(!stream.feed(bad.data(),bad.size(),150,got));
  assert(stream.rejected()>before_rejected);

  // A partial packet becomes stale after 120ms. The next complete packet
  // must decode independently of stale bytes.
  stream.reset();
  assert(!stream.feed(wire.data(),12,200,got));
  assert(stream.feed(wire.data(),wire.size(),500,got));
  assert(got.command==0x22F1);

  // Concatenated messages: return first and retain up to two more.
  std::vector<uint8_t> together;
  for(unsigned i=0;i<4;++i)
    together.insert(together.end(),wire.begin(),wire.end());
  stream.reset();
  const auto overflow_before=stream.overflows();
  assert(stream.feed(together.data(),together.size(),800,got));
  assert(got.command==0x22F1);
  assert(stream.take_pending(got) && got.command==0x22F1);
  assert(stream.take_pending(got) && got.command==0x22F1);
  assert(!stream.take_pending(got));
  assert(stream.overflows()==overflow_before+1); // fourth frame dropped

  // Long noise never creates bogus valid messages and never grows unbounded.
  const uint8_t garbage[16]={0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
                              0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
  stream.reset();
  for(unsigned i=0;i<1000;++i) {
    assert(!stream.feed(garbage,sizeof(garbage),1000+i,got));
    assert(stream.buffered_bytes()<=UartStreamFramer::kCapacity);
  }

  // A message without terminator cannot occupy unlimited memory.
  stream.reset();
  uint8_t head[]={0x33,0x55,0x53};
  assert(!stream.feed(head,sizeof(head),3000,got));
  std::vector<uint8_t> unterminated(600,0xaa);
  const auto overflow_pre=stream.overflows();
  assert(!stream.feed(unterminated.data(),unterminated.size(),3001,got));
  assert(stream.overflows()>overflow_pre);
  assert(stream.buffered_bytes()<=UartStreamFramer::kCapacity);
  stream.reset();
  assert(stream.buffered_bytes()==0 && stream.pending_frames()==0);
  std::puts("Native RAMSES UART stream framer: PASS");
}
