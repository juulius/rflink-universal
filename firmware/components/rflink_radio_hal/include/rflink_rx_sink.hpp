#pragma once
#include <cstddef>
#include <cstdint>
#include "rflink_radio_hal.hpp"
namespace rflink {
// All callbacks run in the RX worker, never from the RMT ISR. Data is valid
// for this call only; ownership remains in the hardware layer.
class RxFrameSink {
 public:
  virtual ~RxFrameSink() = default;
  virtual void on_packet(RadioId radio, uint32_t frequency_hz,
                         const uint8_t* bytes, size_t count) = 0;
  // Signed microsecond periods, high positive and low negative.
  virtual void on_pulses(RadioId radio, uint32_t frequency_hz,
                         const int32_t* durations_us, size_t count) = 0;
};
} // namespace rflink
