#pragma once
// Bounded, hardware-independent RAMSES-II UART byte stream framer.
//
// This is a native ESP-IDF port of the legacy Radio Gaga v0.6.6
// RamsesUartStream. It does NOT configure UART, CC1101, GPIO or radio modes.
// The RF transport must first deliver actual 8N1 bytes from a validated
// physical GDO/UART configuration. It cannot decode arbitrary FSK bitstreams.
#include <array>
#include <cstddef>
#include <cstdint>
#include "rflink_ramses_codec.hpp"

namespace rflink_ramses_ii {

class UartStreamFramer {
 public:
  static constexpr size_t kCapacity = 240;
  static constexpr uint32_t kIdleTimeoutMs = 120;
  static constexpr size_t kPendingCapacity = 2;

  // Returns the first validated frame in 'out'. Further frames from the same
  // read are retained in a bounded pending queue. Invalid frames never escape.
  bool feed(const uint8_t* data, size_t size, uint32_t now_ms, Frame& out);
  bool take_pending(Frame& out);

  // For mode changes/stop: discard incomplete bytes and queued frames.
  // Statistics remain cumulative until the object is destroyed.
  void reset();

  uint32_t rejected() const { return rejected_; }
  uint32_t overflows() const { return overflow_; }
  size_t buffered_bytes() const { return count_; }
  size_t pending_frames() const { return pending_count_; }

 private:
  void reset_bytes();
  std::array<uint8_t, kCapacity> bytes_{};
  std::array<Frame, kPendingCapacity> pending_{};
  size_t count_ = 0;
  size_t pending_count_ = 0;
  uint32_t last_byte_ms_ = 0;
  uint32_t rejected_ = 0;
  uint32_t overflow_ = 0;
};

}  // namespace rflink_ramses_ii
