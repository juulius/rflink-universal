#pragma once
// Pure receive-side instrumentation for the native 868-MHz CC1101 FIFO.
// It observes polls/status bytes only; it never changes radio configuration,
// issues SPI commands, or infers a valid over-the-air RF packet.
#include <cstdint>
#include <limits>

namespace rflink {

struct Rx868PollTelemetry {
  uint32_t polls = 0;
  uint32_t valid_status_polls = 0;
  uint32_t empty_fifo_polls = 0;
  uint32_t partial_fifo_polls = 0; // 1..62 bytes, not a complete 63-byte read.
  uint32_t ready_fifo_polls = 0;   // >=63 bytes, before an attempted FIFO burst.
  uint32_t gaps_over_20ms = 0;
  uint32_t last_gap_us = 0;
  uint32_t max_gap_us = 0;
  uint8_t last_fifo_bytes = 0; // Most recent non-overflow status, not necessarily current.
  uint8_t peak_fifo_bytes = 0; // Max on non-overflow samples only.
};

inline void observe_rx868_poll(Rx868PollTelemetry& stats, uint64_t now_us,
                               uint64_t& prior_poll_us) {
  ++stats.polls;
  if (prior_poll_us && now_us >= prior_poll_us) {
    const uint64_t elapsed = now_us - prior_poll_us;
    const uint32_t gap = elapsed > UINT32_MAX ? UINT32_MAX
                                              : static_cast<uint32_t>(elapsed);
    stats.last_gap_us = gap;
    if (gap > stats.max_gap_us) stats.max_gap_us = gap;
    if (gap > 20000u) ++stats.gaps_over_20ms;
  }
  prior_poll_us = now_us;
}

inline void observe_rx868_status(Rx868PollTelemetry& stats, uint8_t marcstate,
                                 uint8_t rxbytes, uint8_t packet_bytes = 63) {
  ++stats.valid_status_polls;
  // In overflow RXBYTES low bits are not a usable occupancy sample.
  // MARCSTATE 0x11 is RXFIFO_OVERFLOW; do not report 65 as a 65-byte FIFO.
  if ((rxbytes & 0x80u) || ((marcstate & 0x1fu) == 0x11u)) return;
  const uint8_t occupancy = rxbytes & 0x7fu;
  stats.last_fifo_bytes = occupancy;
  if (occupancy > stats.peak_fifo_bytes)
    stats.peak_fifo_bytes = occupancy;
  if (!occupancy) ++stats.empty_fifo_polls;
  else if (occupancy >= packet_bytes) ++stats.ready_fifo_polls;
  else ++stats.partial_fifo_polls;
}

}  // namespace rflink
