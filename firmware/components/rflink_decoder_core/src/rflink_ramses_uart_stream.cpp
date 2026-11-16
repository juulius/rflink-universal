#include "rflink_ramses_uart_stream.hpp"

namespace rflink_ramses_ii {

void UartStreamFramer::reset_bytes() {
  count_ = 0;
  last_byte_ms_ = 0;
}

void UartStreamFramer::reset() {
  reset_bytes();
  pending_count_ = 0;
}

bool UartStreamFramer::take_pending(Frame& out) {
  if (!pending_count_) return false;
  out = pending_[0];
  if (pending_count_ == kPendingCapacity) pending_[0] = pending_[1];
  --pending_count_;
  return true;
}

bool UartStreamFramer::feed(const uint8_t* data, size_t size,
                            uint32_t now_ms, Frame& out) {
  if (!data || !size || size > 1024) return false;

  // Wrap-safe unsigned subtraction, as in the legacy implementation.
  if (count_ && static_cast<uint32_t>(now_ms - last_byte_ms_) > kIdleTimeoutMs)
    reset_bytes();
  last_byte_ms_ = now_ms;

  bool matched = false;
  for (size_t i = 0; i < size; ++i) {
    if (count_ == kCapacity) {
      ++overflow_;
      // Keep only a possible two-byte sync prefix across rollover.
      const uint8_t x = bytes_[count_ - 2];
      const uint8_t y = bytes_[count_ - 1];
      count_ = 0;
      if (x == 0x33 && y == 0x55) {
        bytes_[count_++] = x;
        bytes_[count_++] = y;
      }
    }

    bytes_[count_++] = data[i];
    // RAMSES-II byte stream sync is 33 55 53; FF 00 may precede it.
    // A generic FSK stream must use its own framing policy.
    size_t sync = count_;
    for (size_t j = 0; j + 2 < count_; ++j) {
      if (bytes_[j] == 0x33 && bytes_[j + 1] == 0x55 &&
          bytes_[j + 2] == 0x53) {
        sync = j;
        break;
      }
    }
    if (sync == count_) {
      // Retain at most the unfinished sync prefix, never unbounded noise.
      if (count_ > 2) {
        const uint8_t x = bytes_[count_ - 2];
        const uint8_t y = bytes_[count_ - 1];
        count_ = 0;
        if (x == 0x33 && y == 0x55) {
          bytes_[count_++] = x;
          bytes_[count_++] = y;
        } else if (y == 0x33) {
          bytes_[count_++] = y;
        }
      }
      continue;
    }
    if (sync) {
      for (size_t j = sync; j < count_; ++j)
        bytes_[j - sync] = bytes_[j];
      count_ -= sync;
    }

    // 0x35 is the protocol terminator and cannot be a Manchester nibble.
    // The actual native codec validates UART bytes, Manchester and checksum.
    if (count_ > 3 && data[i] == 0x35) {
      Frame decoded;
      if (decodeWire(bytes_.data(), count_, decoded)) {
        if (!matched) {
          out = decoded;
          matched = true;
        } else if (pending_count_ < pending_.size()) {
          pending_[pending_count_++] = decoded;
        } else {
          ++overflow_;
        }
      } else {
        ++rejected_;
      }
      reset_bytes();
    }
  }
  return matched;
}

}  // namespace rflink_ramses_ii
