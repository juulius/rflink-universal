#include "rflink_sync868_replay.hpp"
#include "rflink_itho_codec.hpp"
#include "rflink_ramses_codec.hpp"
#include <cstdint>
#include <vector>

namespace rflink {
namespace {
constexpr uint32_t kMaxBlocks=256;
constexpr uint32_t kIthoBits=63u*8u;
constexpr uint32_t kMaxRamsesBits=2048;
constexpr uint8_t kIthoHeader[]={0x00,0xB3,0x2A,0xAB,0x2A};

bool valid(const ClockedBitBlock& b) {
  return b.bit_count>0 && b.bit_count<=ClockedBitBlock::kBitCapacity &&
         b.epoch!=0 && b.last_sample_us>=b.first_sample_us;
}
bool contiguous(const ClockedBitBlock& a,const ClockedBitBlock& b) {
  return a.epoch==b.epoch && !a.window_end && !b.discontinuity_before;
}
struct Pair {
  const ClockedBitBlock& first;
  const ClockedBitBlock* second;
  uint32_t available() const {
    return first.bit_count+(second?second->bit_count:0);
  }
  uint8_t bit(uint32_t offset,bool invert) const {
    const auto* block=&first;
    if(offset>=first.bit_count) {
      offset-=first.bit_count;
      block=second;
    }
    return static_cast<uint8_t>(
      ((block->bytes[offset>>3]>>(7u-(offset&7u)))&1u)^unsigned(invert));
  }
  bool uart(uint32_t offset,bool inverted,uint8_t* decoded) const {
    if(!decoded || offset+10u>available() ||
       bit(offset,inverted)!=0 || bit(offset+9u,inverted)!=1)return false;
    uint8_t value=0;
    for(uint32_t i=0;i<8;++i)
      value|=static_cast<uint8_t>(bit(offset+1u+i,inverted)<<i);
    *decoded=value;
    return true;
  }
};
void record(Sync868ReplayReport& report,const Sync868ReplayHit& hit) {
  if(hit.protocol==Sync868ReplayProtocol::itho)++report.itho_structure_only;
  else ++report.ramses_checksum_valid;
  if(report.hit_count<Sync868ReplayReport::kMaxHits)
    report.hits[report.hit_count++]=hit;
  else report.hits_truncated=true;
}
void itho(const Pair& bits,uint32_t current_bits,uint16_t segment,
          uint32_t epoch,uint32_t start_offset,bool invert,
          Sync868ReplayReport& report) {
  if(bits.available()<kIthoBits)return;
  const uint32_t limit=current_bits<=(bits.available()-kIthoBits)?
      current_bits:bits.available()-kIthoBits+1u;
  for(uint32_t at=0;at<limit;++at) {
    bool header=true;
    for(uint32_t i=0;i<sizeof(kIthoHeader)&&header;++i)
      for(uint32_t j=0;j<8;++j)
        if(bits.bit(at+i*8u+j,invert)!=
           ((kIthoHeader[i]>>(7u-j))&1u)) {
          header=false;break;
        }
    if(!header)continue;
    std::vector<uint8_t> packet(63,0);
    for(uint32_t i=0;i<kIthoBits;++i)
      if(bits.bit(at+i,invert))
        packet[i>>3]|=static_cast<uint8_t>(0x80u>>(i&7u));
    itho::Decoded decoded{};
    if(!itho::decode_packet(packet,decoded))continue;
    Sync868ReplayHit hit{};
    hit.protocol=Sync868ReplayProtocol::itho;
    hit.segment=segment;hit.epoch=epoch;
    hit.bit_offset=start_offset+at;hit.span_bits=kIthoBits;
    hit.inverted=invert;
    switch(decoded.type) {
      case itho::MessageType::SPEED:
        hit.detail=Sync868ReplayDetail::speed;hit.value=decoded.speed;break;
      case itho::MessageType::TIMER:
        hit.detail=Sync868ReplayDetail::timer_minutes;
        hit.value=decoded.timer_minutes;break;
      case itho::MessageType::UNIT_SPEED:
        hit.detail=Sync868ReplayDetail::fan_percent;
        hit.value=decoded.fan_percent;break;
      case itho::MessageType::HUMIDITY:
        hit.detail=Sync868ReplayDetail::humidity_percent;
        hit.value=decoded.humidity_percent;break;
      default:break;
    }
    record(report,hit);
    at+=kIthoBits-1u; // No duplicate overlapping structural interpretations.
  }
}
void ramses(const Pair& bits,uint32_t current_bits,uint16_t segment,
            uint32_t epoch,uint32_t start_offset,bool invert,
            Sync868ReplayReport& report) {
  if(bits.available()<160)return;
  for(uint32_t at=0;at<current_bits && at+30u<=bits.available();++at){
    uint8_t a=0,b=0,c=0;
    if(!bits.uart(at,invert,&a)||a!=0x33 ||
       !bits.uart(at+10u,invert,&b)||b!=0x55 ||
       !bits.uart(at+20u,invert,&c)||c!=0x53)continue;
    // Candidate terminates on THIS UART-framed 0x35, never an unrelated
    // later sync. decodeWire validates native Manchester and checksum.
    uint8_t wire[205]{a,b,c};
    size_t size=3;
    uint32_t end=0;
    for(uint32_t pos=at+30u;pos+10u<=bits.available() &&
        pos+10u-at<=kMaxRamsesBits && size<sizeof(wire);pos+=10u) {
      uint8_t value=0;
      if(!bits.uart(pos,invert,&value))break;
      wire[size++]=value;
      if(value==0x35){end=pos+10u;break;}
    }
    if(!end)continue;
    rflink_ramses_ii::Frame decoded{};
    if(!rflink_ramses_ii::decodeWire(wire,size,decoded))continue;
    Sync868ReplayHit hit{};
    hit.protocol=Sync868ReplayProtocol::ramses_ii;
    hit.detail=Sync868ReplayDetail::command;
    hit.value=decoded.command;
    hit.segment=segment;hit.epoch=epoch;
    hit.bit_offset=start_offset+at;
    hit.span_bits=static_cast<uint16_t>(end-at);
    hit.inverted=invert;
    record(report,hit);
    at=end-1u;
  }
}
void label_overlaps(Sync868ReplayReport& r) {
  for(uint32_t i=0;i<r.hit_count;++i) {
    auto& m=r.hits[i];
    if(m.protocol!=Sync868ReplayProtocol::ramses_ii)continue;
    for(uint32_t j=0;j<r.hit_count;++j) {
      const auto& n=r.hits[j];
      if(n.protocol!=Sync868ReplayProtocol::itho ||
         n.segment!=m.segment || n.inverted!=m.inverted)continue;
      const uint32_t mEnd=m.bit_offset+m.span_bits;
      const uint32_t nEnd=n.bit_offset+n.span_bits;
      if(m.bit_offset<nEnd && n.bit_offset<mEnd) {
        m.overlapping=true;r.hits[j].overlapping=true;
        if(m.bit_offset>=n.bit_offset && mEnd<=nEnd)
          m.embedded_in_itho=true;
      }
    }
  }
  for(uint32_t i=0;i<r.hit_count;++i) {
    const auto& m=r.hits[i];
    if(m.overlapping)++r.overlapping_matches;
    if(m.protocol==Sync868ReplayProtocol::ramses_ii){
      if(m.embedded_in_itho)++r.ramses_embedded_in_itho;
      else ++r.ramses_outside_itho;
    }
  }
  // With truncated hits, embedded/outside/overlap counts are partial;
  // the HTTP response explicitly includes hits_truncated to warn users.
}
} // namespace

bool replay_sync868_frozen(uint32_t block_count,Sync868ReplayRead reader,
                          void* context,bool try_inverted,
                          Sync868ReplayReport* result,
                          Sync868ReplayYield progress,void* progress_context) {
  if(!reader||!result||!block_count||block_count>kMaxBlocks)return false;
  Sync868ReplayReport report{};
  report.blocks=block_count;
  report.tried_inverted=try_inverted;
  ClockedBitBlock prior{};
  bool has_prior=false;
  uint16_t segment=0;
  uint32_t segment_offset=0;
  for(uint32_t index=0;index<block_count;++index) {
    ClockedBitBlock current{}, next{};
    if(!reader(context,index,&current)||!valid(current))return false;
    if(index+1u<block_count &&
       (!reader(context,index+1u,&next)||!valid(next)))return false;
    const bool new_segment=!has_prior||!contiguous(prior,current);
    if(new_segment){++report.segments;segment=static_cast<uint16_t>(report.segments-1);
      segment_offset=0;}
    if(current.discontinuity_before)++report.discontinuities;
    Pair view{current,(index+1u<block_count && contiguous(current,next))?
                        &next:nullptr};
    // In a full block the scan may use at most the NEXT contiguous block
    // for look-ahead; 2,048 bits covers every supported candidate.
    for(unsigned polarity=0;polarity<(try_inverted?2u:1u);++polarity) {
      itho(view,current.bit_count,segment,current.epoch,
           segment_offset,polarity!=0,report);
      ramses(view,current.bit_count,segment,current.epoch,
             segment_offset,polarity!=0,report);
    }
    report.recorded_bits+=current.bit_count;
    segment_offset+=current.bit_count;
    prior=current;has_prior=true;
    if(progress)progress(progress_context,index+1u);
  }
  label_overlaps(report);
  *result=report;
  return true;
}
} // namespace rflink
