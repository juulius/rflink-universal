#pragma once
#include <cstddef>
#include <cstdint>
#include "rflink_radio_hal.hpp"
namespace rflink {
// Best-effort status-byte snapshot immediately after a CC1101 FIFO read.
struct RxPacketMetadata {
  bool has_quality=false;
  int16_t rssi_dbm_x2=0; // RSSI in 0.5 dBm units (signed raw RSSI minus offset).
  uint8_t lqi=0;         // CC1101 LQI[6:0]; CRC-OK bit is NOT valid if CRC disabled.
  int32_t frequency_offset_hz=0; // Signed FREQEST * 26MHz / 2^14.
};
// All callbacks run in the RX worker, never from the RMT ISR. Data is valid
// for this call only; ownership remains in the hardware layer.
class RxFrameSink {
 public:
  virtual ~RxFrameSink() = default;
  virtual void on_packet(RadioId radio, uint32_t frequency_hz,
                         const uint8_t* bytes, size_t count) = 0;
  // Optional v0.6.6 parity hook; existing sinks retain the old API.
  virtual void on_packet_with_metadata(RadioId radio,uint32_t frequency_hz,
      const uint8_t* bytes,size_t count,const RxPacketMetadata&) {
    on_packet(radio,frequency_hz,bytes,count);
  }
  // Signed microsecond periods, high positive and low negative.
  virtual void on_pulses(RadioId radio, uint32_t frequency_hz,
                         const int32_t* durations_us, size_t count) = 0;
};
} // namespace rflink
