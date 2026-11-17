// Host-only acceptance for a fixed synchronous 868 MHz capture transport.
// Uses the real native Itho and RAMSES codecs; all RF samples are synthetic.
// NO GPIO/SPI/UART interaction. This is not proof of on-air reception.
#include "rflink_clocked_bit_capture.hpp"
#include "rflink_itho_codec.hpp"
#include "rflink_ramses_codec.hpp"
#include "rflink_ramses_tx.hpp"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>

using rflink::ClockedBitCapture;
using rflink::ClockedBitBlock;

static bool bit_at(const uint8_t* data,size_t i) {
  return (data[i>>3] & (0x80u>>(i&7u)))!=0;
}

static void test_ramses_on_air_bitstream() {
  rflink_ramses_ii::Frame f;
  constexpr uint32_t controller=(37u<<18)|0x1234u;
  constexpr uint32_t fan=(32u<<18)|0x5678u;
  assert(rflink_ramses_ii::tx::buildCommand({controller,fan},
           rflink_ramses_ii::tx::Action::AUTO,f));
  std::vector<uint8_t> bits;
  size_t bit_count=0;
  assert(rflink_ramses_ii::tx::encodeOnAir(f,2,bits,bit_count));
  assert(bit_count>=128 && bit_count<=ClockedBitBlock::kBitCapacity);
  ClockedBitCapture capture;
  assert(capture.begin_epoch(7));
  for(size_t i=0;i<bit_count;++i)
    assert(capture.append(bit_at(bits.data(),i),50000+i*26,7));
  capture.end_window();
  ClockedBitBlock block;
  assert(capture.pop(block));
  assert(block.epoch==7 && block.window_end && !block.discontinuity_before);
  assert(block.bit_count==bit_count);
  assert(!capture.pop(block));
  rflink_ramses_ii::Frame decoded;
  assert(rflink_ramses_ii::decodePacked(block.bytes.data(),
         (block.bit_count+7)/8,decoded));
  assert(decoded.command==0x22F1);
  assert(decoded.addr[0]==controller && decoded.addr[1]==fan);
  // A real modem's bit timing and polarity have NOT been validated here.
}

static void test_itho_unaligned_bits() {
  using namespace rflink;
  const auto logical=itho::build_speed_payload(0x16,{{0x11,0x22,0x33}},1,
                                              itho::SpeedCommand::LOW);
  const auto wire=itho::encode_payload(logical,0xAC);
  assert(wire.size()==63);
  itho::Decoded baseline{};
  assert(itho::decode_packet(wire,baseline) && baseline.speed==2);

  // Include three leading unaligned bits: the transport must preserve them,
  // not silently claim the old CC1101 FIFO byte alignment.
  ClockedBitCapture capture;
  assert(capture.begin_epoch(21));
  for(unsigned i=0;i<3;++i)assert(capture.append((i&1u)!=0,1000+i*26,21));
  for(size_t i=0;i<wire.size()*8;++i)
    assert(capture.append(bit_at(wire.data(),i),1078+i*26,21));
  capture.end_window();
  ClockedBitBlock block{};
  assert(capture.pop(block) && block.bit_count==507 && block.window_end);
  std::vector<uint8_t> realigned(63,0);
  for(size_t i=0;i<wire.size()*8;++i)
    if(bit_at(block.bytes.data(),i+3))
      realigned[i>>3] |= static_cast<uint8_t>(0x80u>>(i&7u));
  assert(realigned==wire);
  itho::Decoded tested{};
  assert(itho::decode_packet(realigned,tested) && tested.speed==2);
}

static void test_drop_newest_and_recovery() {
  ClockedBitCapture capture;
  ClockedBitBlock block{};
  assert(!capture.append(true,1,1));
  assert(!capture.begin_epoch(0));
  assert(capture.begin_epoch(1));
  assert(capture.append(true,1000,1));
  assert(capture.append(false,1026,1));
  capture.report_lost_edges(3);
  assert(capture.buffered_bits()==0);
  assert(capture.stats().reported_missing_edges==3);
  assert(capture.stats().abandoned_bits==2);

  for(size_t n=0;n<3;++n){
    for(size_t i=0;i<ClockedBitBlock::kBitCapacity;++i)
      assert(capture.append((i&1u)!=0,10000+n*100000+i*26,1));
  }
  assert(capture.pending_blocks()==2);
  assert(capture.stats().published_blocks==2);
  assert(capture.stats().dropped_blocks==1);
  assert(capture.stats().dropped_bits==ClockedBitBlock::kBitCapacity);
  assert(capture.pop(block));
  assert(block.bit_count==2048 && block.discontinuity_before);
  assert(capture.pop(block));
  assert(!block.discontinuity_before);
  assert(!capture.pop(block));
  for(size_t i=0;i<16;++i)
    assert(capture.append(true,500000+i*26,1));
  capture.end_window();
  assert(capture.pop(block) && block.bit_count==16);
  assert(block.discontinuity_before && block.window_end);
  // Never permit stale samples from a previous lease to mix with a new one.
  assert(capture.append(true,600000,1));
  assert(capture.begin_epoch(2));
  assert(capture.stats().epoch_resets==1);
  assert(!capture.append(false,600026,1));
  assert(capture.append(false,700000,2));
  capture.end_window();
  assert(capture.pop(block) && block.epoch==2);
  assert(block.bit_count==1 && !block.discontinuity_before);
  assert(capture.stats().rejected_samples==2);

  // Non-monotonic capture timestamp is a discontinuity, not a fake long gap.
  assert(capture.append(true,701000,2));
  assert(!capture.append(false,700999,2));
  assert(capture.stats().clock_regressions==1);
  assert(capture.append(true,702000,2));
  capture.end_window();
  assert(capture.pop(block) && block.discontinuity_before);
}

int main() {
  test_ramses_on_air_bitstream();
  test_itho_unaligned_bits();
  test_drop_newest_and_recovery();
  std::puts("PASS fixed clocked 868-bit stream: synthetic RAMSES+Itho, epoch, loss, bounds");
}
