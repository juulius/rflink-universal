#pragma once
#include <cstdint>

namespace rflink {
// ESP32-S3 RMT RX hardware uses an 8-bit glitch-filter threshold measured
// against the RMT source-clock ticks, NOT the requested 1 MHz symbol clock.
// ESPHome remote_receiver_rmt.cpp makes the same source-clock adjustment.
// The 30 us Radio Gaga v0.6.6 *configured* filter therefore becomes a
// smaller actual hardware filter on ESP32-S3 (3187 ns at 80 MHz).
// This conservative floor prevents rmt_receive() returning
// ESP_ERR_INVALID_ARG (0x102) when 30,000 ns exceeds the 255 tick limit.
constexpr uint32_t kRmtGlitchFilterMaxTicks = 255;
constexpr uint32_t kRmtRequestedFilterNs = 30000;
constexpr uint32_t kRmtIdleTimeoutNs = 5000000;

constexpr uint32_t rmt_effective_filter_ns(uint32_t requested_ns,
                                             uint32_t source_clock_hz) {
  if (source_clock_hz == 0) return 0;  // caller must treat unknown clock as error
  const uint64_t limit_ns =
      (uint64_t{kRmtGlitchFilterMaxTicks} * 1000000000ULL) / source_clock_hz;
  // Compare in 64 bits so an unexpected very slow clock cannot overflow the
  // uint32_t conversion and silently reduce the glitch threshold.
  return uint64_t{requested_ns} < limit_ns
             ? requested_ns : static_cast<uint32_t>(limit_ns);
}
}  // namespace rflink
