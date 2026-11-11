#pragma once
// Native, allocation-free 433 MHz pulse cleanup. Mirrors the *behavior*
// of ESPHome remote_receiver's post-RMT filter: merge pulses shorter than
// the requested threshold with the preceding run, and coalesce same-level
// runs. This is distinct from ESP-IDF's hardware glitch-filter threshold.
#include <cstddef>
#include <cstdint>

namespace rflink {
// In-place: output length is never greater than input length.
// Pulses are signed duration in microseconds, positive = high, negative = low.
// Caller owns the buffer; never invoked from an ISR.
inline size_t filter_short_pulses_inplace(int32_t* pulses, size_t count,
                                          uint32_t min_pulse_us) {
  if (!pulses || !count) return 0;
  size_t write = 0;
  int64_t duration = 0;
  bool level = false;
  for (size_t read = 0; read < count; ++read) {
    const int32_t pulse = pulses[read];
    if (pulse == 0) continue;
    const bool next_level = pulse > 0;
    const int64_t next_duration = pulse > 0
        ? static_cast<int64_t>(pulse)
        : -static_cast<int64_t>(pulse);
    if (duration == 0) {
      duration = next_duration;
      level = next_level;
    } else if (next_level == level || next_duration < min_pulse_us) {
      duration += next_duration;
    } else {
      if (duration >= min_pulse_us) {
        pulses[write++] = level ? static_cast<int32_t>(duration)
                                : -static_cast<int32_t>(duration);
      }
      duration = next_duration;
      level = next_level;
    }
  }
  if (duration >= min_pulse_us) {
    pulses[write++] = level ? static_cast<int32_t>(duration)
                            : -static_cast<int32_t>(duration);
  }
  return write;
}
// ESPHome remote_receiver adds an explicit 5 ms idle-level duration
// after a complete, reconstructed RMT frame. This synthetic duration is
// NOT another RF edge. The last *raw* GDO2 level determines its sign.
// Call from the RX worker, never the ISR. No heap/stack allocations.
inline size_t append_idle_termination_inplace(int32_t* pulses, size_t count,
                                               size_t capacity, uint32_t idle_us,
                                               bool last_raw_level_high) {
  if (!pulses || !count || count >= capacity || !idle_us ||
      idle_us > uint32_t(INT32_MAX)) return count;
  pulses[count] = last_raw_level_high ? -static_cast<int32_t>(idle_us)
                                      : static_cast<int32_t>(idle_us);
  return count + 1;
}
}  // namespace rflink
