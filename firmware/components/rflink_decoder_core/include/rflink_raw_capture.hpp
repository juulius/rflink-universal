#pragma once
// Volatile, bounded v0.6.6-style raw RF evidence (never written to NVS).
// Allocate this large object in ESP32-S3 PSRAM, NOT on the worker/HTTP stack.
#include <array>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include "rflink_rx_sink.hpp"

namespace rflink {
constexpr size_t kRawCaptureDepthPerRadio=8;
constexpr size_t kRawCaptureMaxPulses=1024; // 512 RMT symbols, 2 durations each
constexpr size_t kRawCaptureMaxBytes=63;
enum class RawCaptureKind : uint8_t { none=0, pulses=1, packet=2 };
struct RawCaptureEntry {
  uint32_t sequence=0;  // Exactly the native RX event sequence, not log sequence.
  uint32_t uptime_ms=0;
  uint32_t frequency_hz=0;
  uint16_t count=0;
  uint8_t radio=255;
  RawCaptureKind kind=RawCaptureKind::none;
  // Signed durations are data on the GDO2 pin; these are NOT decoded bytes.
  std::array<int32_t,kRawCaptureMaxPulses> pulses_us{};
  // Original CC1101 FIFO bytes on 868.30 MHz (may be undecodable/noise).
  std::array<uint8_t,kRawCaptureMaxBytes> packet{};
};
class RawCaptureHistory {
 public:
  RawCaptureHistory()=default;
  RawCaptureHistory(const RawCaptureHistory&)=delete;
  RawCaptureHistory& operator=(const RawCaptureHistory&)=delete;

  void save_pulses(uint32_t sequence,uint32_t uptime_ms,RadioId radio,
                   uint32_t frequency_hz,const int32_t* durations,size_t count) {
    if(!durations || count<2 || count>kRawCaptureMaxPulses ||
       static_cast<unsigned>(radio)>1 || !sequence)return;
    std::lock_guard<std::mutex> lock(mutex_);
    auto& slot=next(radio);
    slot.sequence=sequence;
    slot.uptime_ms=uptime_ms;
    slot.frequency_hz=frequency_hz;
    slot.radio=static_cast<uint8_t>(radio);
    slot.kind=RawCaptureKind::pulses;
    slot.count=static_cast<uint16_t>(count);
    std::copy_n(durations,count,slot.pulses_us.begin());
  }
  void save_packet(uint32_t sequence,uint32_t uptime_ms,RadioId radio,
                   uint32_t frequency_hz,const uint8_t* data,size_t count) {
    if(!data || count==0 || count>kRawCaptureMaxBytes ||
       radio!=RadioId::rf86830 || !sequence)return;
    std::lock_guard<std::mutex> lock(mutex_);
    auto& slot=next(radio);
    slot.sequence=sequence;
    slot.uptime_ms=uptime_ms;
    slot.frequency_hz=frequency_hz;
    slot.radio=static_cast<uint8_t>(radio);
    slot.kind=RawCaptureKind::packet;
    slot.count=static_cast<uint16_t>(count);
    std::copy_n(data,count,slot.packet.begin());
  }

  // Caller owns the destination (typically allocated in PSRAM). Never return
  // pointers into the ring: the receive task may overwrite a slot after unlock.
  bool copy(uint8_t radio,uint32_t sequence,RawCaptureEntry* output) const {
    if(!output || radio>=3 || !sequence)return false;
    std::lock_guard<std::mutex> lock(mutex_);
    const auto& slots=entries_[radio];
    for(const auto& entry:slots){
      if(entry.sequence==sequence && entry.radio==radio &&
         entry.kind!=RawCaptureKind::none){
        *output=entry;
        return true;
      }
    }
    return false;
  }
  static constexpr size_t allocated_payload_bytes(){
    return sizeof(RawCaptureHistory);
  }
 private:
  RawCaptureEntry& next(RadioId radio){
    const auto idx=static_cast<size_t>(radio);
    const size_t slot=cursor_[idx]++ % kRawCaptureDepthPerRadio;
    return entries_[idx][slot];
  }
  mutable std::mutex mutex_;
  std::array<std::array<RawCaptureEntry,kRawCaptureDepthPerRadio>,3> entries_{};
  std::array<uint32_t,3> cursor_{};
};
} // namespace rflink
