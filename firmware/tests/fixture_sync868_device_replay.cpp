// Synthetic-only regression of the ESP32's bounded, native 868 bit replay.
// Compiled as a host executable against the SAME Itho/RAMSES firmware codecs.
#include "rflink_sync868_replay.hpp"
#include "rflink_itho_codec.hpp"
#include "rflink_ramses_codec.hpp"
#include "rflink_ramses_tx.hpp"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>

using rflink::ClockedBitBlock;
using rflink::Sync868ReplayReport;
static void append(std::vector<uint8_t>& bits,const std::vector<uint8_t>& bytes,size_t nbits){
  for(size_t i=0;i<nbits;++i)bits.push_back((bytes[i>>3]>>(7u-(i&7u)))&1u);
}
static std::vector<ClockedBitBlock> blocks(const std::vector<uint8_t>& bits){
  std::vector<ClockedBitBlock> out;
  for(size_t start=0;start<bits.size();start+=2048){
    ClockedBitBlock b{};
    b.bit_count=static_cast<uint16_t>(
      bits.size()-start>2048?2048:bits.size()-start);
    b.epoch=7;
    b.first_sample_us=uint64_t(start)*26u+1000u;
    b.last_sample_us=b.first_sample_us+uint64_t(b.bit_count-1u)*26u;
    for(size_t i=0;i<b.bit_count;++i)
      if(bits[start+i])b.bytes[i>>3]|=uint8_t(0x80u>>(i&7u));
    b.window_end=start+b.bit_count==bits.size();
    out.push_back(b);
  }
  return out;
}
static bool read(void* ptr,uint32_t i,ClockedBitBlock* out) {
  auto& b=*static_cast<std::vector<ClockedBitBlock>*>(ptr);
  if(i>=b.size())return false;
  *out=b[i];return true;
}
static Sync868ReplayReport run(std::vector<ClockedBitBlock>& b,bool inverted=false) {
  Sync868ReplayReport result{};
  const bool ok=rflink::replay_sync868_frozen(
    static_cast<uint32_t>(b.size()),read,&b,inverted,&result);
  assert(ok);
  return result;
}
int main(){
  const auto logical=rflink::itho::build_speed_payload(
    0x16,{{0x11,0x22,0x33}},1,rflink::itho::SpeedCommand::LOW);
  const auto itho=rflink::itho::encode_payload(logical,0xAC);
  assert(itho.size()==63);
  const uint32_t controller=(37u<<18)|0x1234u, fan=(32u<<18)|0x5678u;
  rflink_ramses_ii::Frame frame{};
  assert(rflink_ramses_ii::tx::buildCommand(
    {controller,fan},rflink_ramses_ii::tx::Action::AUTO,frame));
  std::vector<uint8_t> ramses;
  size_t count=0;
  assert(rflink_ramses_ii::tx::encodeOnAir(frame,2,ramses,count));

  // Known Itho frame starts in block 0, RAMSES outside Itho in block 1.
  std::vector<uint8_t> all(1800,0);
  append(all,itho,itho.size()*8);
  all.insert(all.end(),6,1);
  append(all,ramses,count);
  auto b=blocks(all);
  assert(b.size()==2);
  const auto result=run(b);
  assert(result.recorded_bits==all.size());
  assert(result.segments==1);
  assert(result.itho_structure_only==1);
  assert(result.ramses_checksum_valid==2);
  assert(result.ramses_embedded_in_itho==1);
  assert(result.ramses_outside_itho==1);
  assert(result.overlapping_matches==2);
  for(uint32_t i=0;i<result.hit_count;++i){
    const auto& h=result.hits[i];
    if(h.protocol==rflink::Sync868ReplayProtocol::itho)
      assert(h.bit_offset==1800&&h.span_bits==504);
  }

  // A UART frame split across a FULL 2,048-bit block must be recognized.
  std::vector<uint8_t> split(1960,0);
  append(split,ramses,count);
  auto contiguous=blocks(split);
  assert(contiguous.size()==2);
  assert(run(contiguous).ramses_checksum_valid==1);
  contiguous[0].window_end=true;
  assert(run(contiguous).ramses_checksum_valid==0);
  contiguous[0].window_end=false;
  contiguous[1].discontinuity_before=true;
  assert(run(contiguous).ramses_checksum_valid==0);
  contiguous[1].discontinuity_before=false;
  contiguous[1].epoch=8;
  assert(run(contiguous).ramses_checksum_valid==0);

  // Complemented signal only matches under explicitly enabled hypothesis.
  for(auto& v:split)v^=1u;
  auto reversed=blocks(split);
  assert(run(reversed).ramses_checksum_valid==0);
  const auto inverted=run(reversed,true);
  assert(inverted.ramses_checksum_valid==1);
  assert(inverted.hits[0].inverted);

  // The on-device scanner must reject malformed, untrusted block metadata.
  reversed[0].bit_count=4095;
  Sync868ReplayReport invalid{};
  assert(!rflink::replay_sync868_frozen(
    static_cast<uint32_t>(reversed.size()),read,&reversed,true,&invalid));
  std::puts("PASS bounded native 868 Itho/RAMSES replay: offsets, overlaps, "
            "cross-block, epochs, loss, inversion and bounds");
}
