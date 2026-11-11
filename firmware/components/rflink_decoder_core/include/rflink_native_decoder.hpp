#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <array>
#include <mutex>
#include "rflink_rx_sink.hpp"
#include "rflink_raw_capture.hpp"
#include "rflink_user_rx_runtime.hpp"
namespace rflink {
enum class RecognizedProtocol : uint8_t { none, itho_rft, ramses_ii };
struct DecodeCounters {
 uint32_t packet_inputs=0;
 uint32_t pulse_inputs_43342=0;
 uint32_t pulse_inputs_43392=0;
 uint32_t itho_messages=0;
 uint32_t ramses_messages=0;
 uint32_t unknown_packet_inputs=0;
 uint32_t unknown_prefix_inputs=0; // 63 bytes read, but not the legacy protocol preamble.
 uint32_t unknown_decode_inputs=0; // Legacy preamble matched; no supported decoder accepted it.
 uint32_t rejected_inputs=0;
 uint32_t overlapping_decodes=0;
 RecognizedProtocol last_protocol=RecognizedProtocol::none;
 uint16_t last_ramses_command=0;
 uint32_t user_rx_matches=0;
 uint32_t user_rx_ambiguous=0;
 uint8_t user_decoders_enabled=0;
 uint8_t user_library_storage_state=255; // UserLibraryReadCode, 255=not loaded
 // Deliberately no RF identities, payload bytes, secrets or rolling counters.
};
// Diagnostic records intentionally omit RF payloads, device IDs and rolling codes.
// All callbacks run in the worker; API reads occur from the HTTP task.
enum class EventKind : uint8_t { raw_pulses, unknown_packet, itho, ramses, user_match, user_ambiguous };
struct RxRecentEvent {
 uint32_t sequence=0;
 uint32_t uptime_ms=0;
 RadioId radio=RadioId::rf43342;
 EventKind kind=EventKind::unknown_packet;
 uint16_t count=0;
 uint16_t command=0;
 bool overlapping=false;
};
constexpr size_t kRecentEventCapacity=32;
class NativeDecoder final : public RxFrameSink {
public:
 void on_packet(RadioId radio, uint32_t frequency_hz,
                const uint8_t* bytes, size_t count) override;
 void on_packet_with_metadata(RadioId radio,uint32_t frequency_hz,
                const uint8_t* bytes,size_t count,
                const RxPacketMetadata& quality) override;
 void on_pulses(RadioId radio, uint32_t frequency_hz,
                const int32_t* durations_us, size_t count) override;
 DecodeCounters snapshot() const;
 // Setup-only calls; must run BEFORE starting any RX worker.
 bool install_user_library(const UserLibrary& library,std::string& error);
 void set_user_library_storage_state(uint8_t code) {
   user_library_storage_state_.store(code,std::memory_order_release);
 }
 using UptimeProvider = uint32_t(*)();
 void set_uptime_provider(UptimeProvider fn) { uptime_provider_.store(fn,std::memory_order_relaxed); }
 // Observers receive metadata only, after the internal event mutex is released.
 // Last argument links a compact log record to an optional volatile raw capture.
 using EventObserver=void(*)(void*,RadioId,EventKind,size_t,uint16_t,bool,uint32_t,uint32_t);
 // Called synchronously while the decoder's RX input is still valid. The
 // optional detail store does not allocate or own the driver's input buffer.
 using DetailObserver=void(*)(void*,RadioId,EventKind,uint32_t,uint32_t,
                               uint32_t,const int32_t*,const uint8_t*,
                               size_t,const RxPacketMetadata*);
 void set_detail_observer(DetailObserver fn,void* context){
   detail_observer_=fn;detail_ctx_=context;
 }
 void set_event_observer(EventObserver fn,void* context) { observer_=fn; observer_ctx_=context; }
 void set_raw_capture_history(RawCaptureHistory* history) { raw_history_=history; }
 bool raw_capture(uint8_t radio,uint32_t seq,RawCaptureEntry* out) const {
   return raw_history_ && raw_history_->copy(radio,seq,out);
 }
 bool raw_capture_enabled() const {return raw_history_!=nullptr;}
 // Returns the most recent events, ordered oldest to newest, bounded by capacity.
 size_t recent_events(RxRecentEvent* out,size_t max_count) const;
private:
 bool try_user_packet(RadioId radio,uint32_t frequency_hz,
                      const uint8_t* bytes,size_t count,
                      const RxPacketMetadata& quality);
 void record_event(RadioId radio,EventKind kind,size_t count,uint16_t command=0,
                   bool overlapping=false,const int32_t* pulses=nullptr,
                   const uint8_t* packet=nullptr,uint32_t frequency_hz=0,
                   const RxPacketMetadata* quality=nullptr);
 UserRxRuntime user_runtime_{};
 std::atomic<uint8_t> user_library_storage_state_{255};
 std::atomic<uint8_t> user_decoders_enabled_{0};
 std::atomic<uint32_t> user_matches_{0},user_ambiguous_{0};
 RawCaptureHistory* raw_history_=nullptr;
 DetailObserver detail_observer_=nullptr;
 void* detail_ctx_=nullptr;
 EventObserver observer_=nullptr;
 void* observer_ctx_=nullptr;
 mutable std::mutex events_lock_;
 std::array<RxRecentEvent,kRecentEventCapacity> events_{};
 uint32_t event_sequence_=0;
 size_t event_used_=0;
 std::atomic<UptimeProvider> uptime_provider_{nullptr};
 std::atomic<uint32_t> packet_inputs_{0}, pulses_a_{0}, pulses_b_{0};
 std::atomic<uint32_t> itho_{0}, ramses_{0}, unknown_{0}, unknown_prefix_{0},
   unknown_decode_{0}, rejected_{0}, overlap_{0};
 std::atomic<uint8_t> last_protocol_{0};
 std::atomic<uint16_t> last_command_{0};
};
} // namespace rflink
