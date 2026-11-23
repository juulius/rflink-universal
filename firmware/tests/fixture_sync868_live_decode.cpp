// Native live sync 868 bridge: real codec fixtures, synthetic bitstream only.
// This is not evidence of a correctly wired/data-clock radio or on-air RX.
#include "rflink_sync868_live.hpp"
#include "rflink_itho_codec.hpp"
#include "rflink_ramses_codec.hpp"
#include "rflink_ramses_tx.hpp"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>

using rflink::ClockedBitBlock;
using rflink::Sync868ReplayHit;

static void append(std::vector<uint8_t>& bits,
                   const std::vector<uint8_t>& bytes,size_t nbits) {
  for(size_t i=0;i<nbits;++i)
    bits.push_back((bytes[i>>3]>>(7u-(i&7u)))&1u);
}
static std::vector<ClockedBitBlock> chunk(const std::vector<uint8_t>& bits) {
  std::vector<ClockedBitBlock> output;
  for(size_t start=0;start<bits.size();start+=2048) {
    ClockedBitBlock b{};
    b.bit_count=static_cast<uint16_t>(
      std::min<size_t>(ClockedBitBlock::kBitCapacity,bits.size()-start));
    b.epoch=11;
    b.first_sample_us=start*26u+100u;
    b.last_sample_us=b.first_sample_us+b.bit_count*26u;
    for(size_t i=0;i<b.bit_count;++i)
      if(bits[start+i])b.bytes[i>>3]|=uint8_t(0x80u>>(i&7u));
    b.window_end=start+b.bit_count==bits.size();
    output.push_back(b);
  }
  return output;
}
static void collect(void* raw,const Sync868ReplayHit& hit) {
  static_cast<std::vector<Sync868ReplayHit>*>(raw)->push_back(hit);
}
int main(){
  const auto logical=rflink::itho::build_speed_payload(
    0x16,{{0x11,0x22,0x33}},1,rflink::itho::SpeedCommand::LOW);
  const auto itho=rflink::itho::encode_payload(logical,0xAC);
  assert(itho.size()==63);
  const uint32_t controller=(37u<<18)|0x1234u,fan=(32u<<18)|0x5678u;
  rflink_ramses_ii::Frame frame{};
  assert(rflink_ramses_ii::tx::buildCommand(
    {controller,fan},rflink_ramses_ii::tx::Action::AUTO,frame));
  std::vector<uint8_t> ramses;
  size_t count=0;
  assert(rflink_ramses_ii::tx::encodeOnAir(frame,2,ramses,count));

  // Complete Itho crossing 2,048-bit boundary, with checksum-validated
  // RAMSES embedded; another independent RAMSES follows.
  std::vector<uint8_t> bits(1800,0);
  append(bits,itho,itho.size()*8);
  bits.insert(bits.end(),6,1);
  append(bits,ramses,count);
  bits.insert(bits.end(),2300,0);
  auto blocks=chunk(bits);
  assert(blocks.size()>=3);
  rflink::Sync868LiveDecoder live;
  std::vector<Sync868ReplayHit> hits;
  for(size_t i=0;i<blocks.size();++i)
    live.consume(blocks[i],static_cast<uint32_t>(i+1),collect,&hits);
  const auto snapshot=live.stats();
  assert(snapshot.blocks==blocks.size());
  assert(snapshot.itho_candidates==1);
  assert(snapshot.ramses_checksum_valid==2);
  assert(snapshot.ramses_embedded==1);
  assert(hits.size()==2); // One Itho physical frame, one outside RAMSES.
  assert(hits[0].protocol==rflink::Sync868ReplayProtocol::itho);
  assert(hits[1].protocol==rflink::Sync868ReplayProtocol::ramses_ii);

  // Inverted polarity remains a marked hypothesis, never a silently
  // inverted RF device command.
  std::vector<uint8_t> complemented(1960,0);
  append(complemented,ramses,count);
  for(auto& bit:complemented)bit^=1u;
  auto inv=chunk(complemented);
  rflink::Sync868LiveDecoder inverted;
  hits.clear();
  inverted.consume(inv[0],10,collect,&hits);
  inverted.consume(inv[1],11,collect,&hits);
  assert(hits.size()==1 && hits[0].inverted);

  // Queue discontinuity: 2 partial fragments that individually lack a full
  // frame must never be falsely joined across a lost queue entry.
  std::vector<uint8_t> split(1960,0);
  append(split,ramses,count);
  auto broken=chunk(split);
  rflink::Sync868LiveDecoder dropped;
  hits.clear();
  dropped.consume(broken[0],2,collect,&hits);
  dropped.note_queue_drop();
  dropped.consume(broken[1],4,collect,&hits);
  assert(hits.empty());
  assert(dropped.stats().queue_drops==1);
  assert(dropped.stats().discontinuities>=1);

  // Early partial terminal windows cannot leak into a following new epoch.
  auto interrupted=chunk(split);
  interrupted[0].window_end=true;
  interrupted[1].epoch=12;
  rflink::Sync868LiveDecoder stopped;
  hits.clear();
  stopped.consume(interrupted[0],1,collect,&hits);
  stopped.consume(interrupted[1],2,collect,&hits);
  assert(hits.empty());

  std::puts("PASS sync868 live codec: no duplicate nested RF messages, "
    "cross-block, polarity, queue drop, discontinuity, epoch and window");
}
