#include "rflink_native_decoder.hpp"
#include "rflink_itho_codec.hpp"
#include "rflink_ramses_codec.hpp"
#include <vector>
#include <algorithm>
namespace rflink {
void NativeDecoder::record_event(RadioId radio,EventKind kind,size_t count,uint16_t command,bool overlapping){
 const auto clock=uptime_provider_.load(std::memory_order_relaxed);
 const uint32_t ms=clock?clock():0;
 std::lock_guard<std::mutex> lock(events_lock_);
 const uint32_t seq=++event_sequence_;
 events_[(seq-1)%kRecentEventCapacity]={seq,ms,radio,kind,static_cast<uint16_t>(count),command,overlapping};
 if(event_used_<kRecentEventCapacity)++event_used_;
}
size_t NativeDecoder::recent_events(RxRecentEvent* out,size_t max_count) const {
 if(!out || max_count==0)return 0;
 std::lock_guard<std::mutex> lock(events_lock_);
 const size_t n=std::min(event_used_,max_count);
 for(size_t i=0;i<n;++i){
   const size_t ix=(event_sequence_-n+i)%kRecentEventCapacity;
   out[i]=events_[ix];
 }
 return n;
}

void NativeDecoder::on_packet(RadioId radio, uint32_t frequency_hz,
                              const uint8_t* bytes, size_t count) {
 if (radio != RadioId::rf86830 || frequency_hz < 868200000 ||
     frequency_hz > 868400000 || !bytes || count != 63) {
   rejected_.fetch_add(1, std::memory_order_relaxed); return;
 }
 packet_inputs_.fetch_add(1, std::memory_order_relaxed);
 // Both protocol families share a preamble; do not classify RF noise as RF messages.
 static const uint8_t prefix[] = {0x00,0xB3,0x2A,0xAB,0x2A};
 bool possible=true;
 for(size_t i=0;i<sizeof(prefix);++i) if(bytes[i]!=prefix[i]){possible=false;break;}
 if (!possible){unknown_.fetch_add(1, std::memory_order_relaxed);record_event(radio,EventKind::unknown_packet,count);return;}
 // Both families may be valid for the very same packet. This is observed
 // with real v0.6.6 RAMSES captures and an Itho speed command. Do not emit two
 // device events: keep the legacy decoder precedence, but count collisions.
 rflink_ramses_ii::Frame ramses;
 const bool is_ramses=rflink_ramses_ii::decodePacked(bytes,count,ramses);
 itho::Decoded itho_frame;
 const std::vector<uint8_t> packet(bytes,bytes+count);
 const bool is_itho=itho::decode_packet(packet,itho_frame) && itho_frame.valid;
 if(is_ramses && is_itho)overlap_.fetch_add(1,std::memory_order_relaxed);
 if(is_ramses){
   ramses_.fetch_add(1,std::memory_order_relaxed);
   last_command_.store(ramses.command,std::memory_order_relaxed);
   last_protocol_.store(static_cast<uint8_t>(RecognizedProtocol::ramses_ii),std::memory_order_relaxed);
   record_event(radio,EventKind::ramses,count,ramses.command,is_itho);
   return;
 }
 if(is_itho){
   itho_.fetch_add(1,std::memory_order_relaxed);
   last_protocol_.store(static_cast<uint8_t>(RecognizedProtocol::itho_rft),std::memory_order_relaxed);
   record_event(radio,EventKind::itho,count);
   return;
 }
 unknown_.fetch_add(1,std::memory_order_relaxed);
 record_event(radio,EventKind::unknown_packet,count);
}
void NativeDecoder::on_pulses(RadioId radio, uint32_t frequency_hz,
                              const int32_t* durations_us, size_t count) {
 if (!durations_us || count<2 || count>512) {
   rejected_.fetch_add(1,std::memory_order_relaxed); return;
 }
 if(radio==RadioId::rf43342 && frequency_hz==433420000){
   pulses_a_.fetch_add(1,std::memory_order_relaxed);record_event(radio,EventKind::raw_pulses,count);
 }else if(radio==RadioId::rf43392 && frequency_hz==433920000){
   pulses_b_.fetch_add(1,std::memory_order_relaxed);record_event(radio,EventKind::raw_pulses,count);
 }else rejected_.fetch_add(1,std::memory_order_relaxed);
 // 433 protocols not ported. Never label a raw RMT capture as a decoded message.
}
DecodeCounters NativeDecoder::snapshot() const {
 DecodeCounters d{};
 d.packet_inputs=packet_inputs_.load(std::memory_order_relaxed);
 d.pulse_inputs_43342=pulses_a_.load(std::memory_order_relaxed);
 d.pulse_inputs_43392=pulses_b_.load(std::memory_order_relaxed);
 d.itho_messages=itho_.load(std::memory_order_relaxed);
 d.ramses_messages=ramses_.load(std::memory_order_relaxed);
 d.unknown_packet_inputs=unknown_.load(std::memory_order_relaxed);
 d.rejected_inputs=rejected_.load(std::memory_order_relaxed);
 d.overlapping_decodes=overlap_.load(std::memory_order_relaxed);
 d.last_protocol=static_cast<RecognizedProtocol>(last_protocol_.load(std::memory_order_relaxed));
 d.last_ramses_command=last_command_.load(std::memory_order_relaxed);
 return d;
}
} // namespace rflink
