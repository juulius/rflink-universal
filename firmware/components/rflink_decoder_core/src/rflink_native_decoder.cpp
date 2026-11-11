#include "rflink_native_decoder.hpp"
#include "rflink_itho_codec.hpp"
#include "rflink_ramses_codec.hpp"
#include <vector>
#include <algorithm>
namespace rflink {
bool NativeDecoder::install_user_library(const UserLibrary& library,std::string& error) {
  const bool installed=user_runtime_.install(library,error);
  if(installed)user_decoders_enabled_.store(static_cast<uint8_t>(user_runtime_.enabled_count()),
                                          std::memory_order_release);
  return installed;
}

bool NativeDecoder::try_user_packet(RadioId radio,uint32_t frequency_hz,
                                    const uint8_t* bytes,size_t count,
                                    const RxPacketMetadata& quality) {
  DecodeResult result{};
  const auto outcome=user_runtime_.match_packet(static_cast<uint8_t>(radio),
                                                 frequency_hz,bytes,count,result);
  if(outcome==UserRxOutcome::none)return false;
  if(outcome==UserRxOutcome::match)user_matches_.fetch_add(1,std::memory_order_relaxed);
  else user_ambiguous_.fetch_add(1,std::memory_order_relaxed);
  record_event(radio,outcome==UserRxOutcome::match?EventKind::user_match:EventKind::user_ambiguous,
               count,0,outcome==UserRxOutcome::ambiguous,nullptr,bytes,frequency_hz,&quality);
  return true;
}

void NativeDecoder::record_event(RadioId radio,EventKind kind,size_t count,uint16_t command,
                                 bool overlapping,const int32_t* pulses,
                                 const uint8_t* packet,uint32_t frequency_hz,
                                 const RxPacketMetadata* quality){
 const auto clock=uptime_provider_.load(std::memory_order_relaxed);
 const uint32_t ms=clock?clock():0;
 uint32_t seq=0;
 {
   std::lock_guard<std::mutex> lock(events_lock_);
   seq=++event_sequence_;
   events_[(seq-1)%kRecentEventCapacity]={seq,ms,radio,kind,static_cast<uint16_t>(count),command,overlapping};
   if(event_used_<kRecentEventCapacity)++event_used_;
 }
 // A separate, bounded PSRAM ring retains v0.6.6-style raw data. Copy
 // synchronously on the RX worker, before its RMT/FIFO buffers are reused.
 if(raw_history_){
   if(pulses)raw_history_->save_pulses(seq,ms,radio,frequency_hz,pulses,count);
   else if(packet)raw_history_->save_packet(seq,ms,radio,frequency_hz,packet,count,
                                            quality?*quality:RxPacketMetadata{});
 }
 // Detail evidence is copied while the RX worker still owns its input.
 // This callback must never allocate or block on HTTP/network operations.
 if(detail_observer_ && (pulses || packet))
   detail_observer_(detail_ctx_,radio,kind,seq,ms,frequency_hz,
                    pulses,packet,count,quality);
 // Keep logging tiny: the event sequence becomes its correlation ID.
 if(observer_)observer_(observer_ctx_,radio,kind,count,command,overlapping,ms,seq);
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

void NativeDecoder::on_packet(RadioId radio,uint32_t frequency_hz,
                              const uint8_t* bytes,size_t count){
  on_packet_with_metadata(radio,frequency_hz,bytes,count,RxPacketMetadata{});
}
void NativeDecoder::on_packet_with_metadata(RadioId radio,uint32_t frequency_hz,
                                           const uint8_t* bytes,size_t count,
                                           const RxPacketMetadata& quality) {
 if (radio != RadioId::rf86830 || frequency_hz < 868200000 ||
     frequency_hz > 868400000 || !bytes || count != 63) {
   rejected_.fetch_add(1, std::memory_order_relaxed); return;
 }
 packet_inputs_.fetch_add(1, std::memory_order_relaxed);
 // Both protocol families share a preamble; do not classify RF noise as RF messages.
 static const uint8_t prefix[] = {0x00,0xB3,0x2A,0xAB,0x2A};
 bool possible=true;
 for(size_t i=0;i<sizeof(prefix);++i) if(bytes[i]!=prefix[i]){possible=false;break;}
 if (!possible){
   if(try_user_packet(radio,frequency_hz,bytes,count,quality))return;
   unknown_.fetch_add(1,std::memory_order_relaxed);
   unknown_prefix_.fetch_add(1,std::memory_order_relaxed);
   // 1=expected legacy preamble absent. No packet bytes/keys in the log.
   record_event(radio,EventKind::unknown_packet,count,1,false,nullptr,bytes,frequency_hz,&quality);return;
 }
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
   record_event(radio,EventKind::ramses,count,ramses.command,is_itho,nullptr,bytes,frequency_hz,&quality);
   return;
 }
 if(is_itho){
   itho_.fetch_add(1,std::memory_order_relaxed);
   last_protocol_.store(static_cast<uint8_t>(RecognizedProtocol::itho_rft),std::memory_order_relaxed);
   record_event(radio,EventKind::itho,count,0,false,nullptr,bytes,frequency_hz,&quality);
   return;
 }
 if(try_user_packet(radio,frequency_hz,bytes,count,quality))return;
 unknown_.fetch_add(1,std::memory_order_relaxed);
 unknown_decode_.fetch_add(1,std::memory_order_relaxed);
 // 2=legacy preamble present, neither Itho nor RAMSES decoder accepted.
 record_event(radio,EventKind::unknown_packet,count,2,false,nullptr,bytes,frequency_hz,&quality);
}
void NativeDecoder::on_pulses(RadioId radio, uint32_t frequency_hz,
                              const int32_t* durations_us, size_t count) {
 if (!durations_us || count<2 || count>1024) {
   rejected_.fetch_add(1,std::memory_order_relaxed); return;
 }
 if(radio==RadioId::rf43342 && frequency_hz==433420000){
   pulses_a_.fetch_add(1,std::memory_order_relaxed);
 }else if(radio==RadioId::rf43392 && frequency_hz==433920000){
   pulses_b_.fetch_add(1,std::memory_order_relaxed);
 }else {rejected_.fetch_add(1,std::memory_order_relaxed);return;}
 DecodeResult decoded{};
 const auto outcome=user_runtime_.match_raw(static_cast<uint8_t>(radio),frequency_hz,
                                             durations_us,count,decoded);
 if(outcome==UserRxOutcome::match)user_matches_.fetch_add(1,std::memory_order_relaxed);
 else if(outcome==UserRxOutcome::ambiguous)user_ambiguous_.fetch_add(1,std::memory_order_relaxed);
 const auto kind=outcome==UserRxOutcome::match?EventKind::user_match:
                 outcome==UserRxOutcome::ambiguous?EventKind::user_ambiguous:EventKind::raw_pulses;
 record_event(radio,kind,count,0,outcome==UserRxOutcome::ambiguous,durations_us,nullptr,frequency_hz);
 // Only an unambiguous *recipe match* is claimed; no user devices or TX yet.
}
DecodeCounters NativeDecoder::snapshot() const {
 DecodeCounters d{};
 d.packet_inputs=packet_inputs_.load(std::memory_order_relaxed);
 d.pulse_inputs_43342=pulses_a_.load(std::memory_order_relaxed);
 d.pulse_inputs_43392=pulses_b_.load(std::memory_order_relaxed);
 d.itho_messages=itho_.load(std::memory_order_relaxed);
 d.ramses_messages=ramses_.load(std::memory_order_relaxed);
 d.unknown_packet_inputs=unknown_.load(std::memory_order_relaxed);
 d.unknown_prefix_inputs=unknown_prefix_.load(std::memory_order_relaxed);
 d.unknown_decode_inputs=unknown_decode_.load(std::memory_order_relaxed);
 d.rejected_inputs=rejected_.load(std::memory_order_relaxed);
 d.overlapping_decodes=overlap_.load(std::memory_order_relaxed);
 d.last_protocol=static_cast<RecognizedProtocol>(last_protocol_.load(std::memory_order_relaxed));
 d.last_ramses_command=last_command_.load(std::memory_order_relaxed);
 d.user_rx_matches=user_matches_.load(std::memory_order_relaxed);
 d.user_rx_ambiguous=user_ambiguous_.load(std::memory_order_relaxed);
 d.user_decoders_enabled=user_decoders_enabled_.load(std::memory_order_acquire);
 d.user_library_storage_state=user_library_storage_state_.load(std::memory_order_acquire);
 return d;
}
} // namespace rflink
