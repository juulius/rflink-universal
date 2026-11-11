#pragma once
// RAMSES-II UART byte stream deframer. Pure C++, no ESP-IDF dependency.
#include <array>
#include <cstddef>
#include <cstdint>
#if __has_include("rflink_decoders/ramses_ii/codec.h")
#include "rflink_decoders/ramses_ii/codec.h"
#else
#include "../../decoders/ramses_ii/codec.h"
#endif
namespace esphome { namespace rflink_universal {
class RamsesUartStream {
 public:
  bool feed(const uint8_t *data, size_t len, uint32_t now_ms, rflink_ramses_ii::Frame &out);
  void reset();
  bool take_pending(rflink_ramses_ii::Frame &out);
  uint32_t rejected() const {return rejected_;}
  uint32_t overflow() const {return overflow_;}
 private:
  static constexpr size_t CAPACITY = 240;
  void reset_buffer_();
  std::array<rflink_ramses_ii::Frame,2> pending_{};
  size_t pending_count_{0};
  std::array<uint8_t,CAPACITY> bytes_{};
  size_t count_{0};
  uint32_t last_byte_ms_{0};
  uint32_t rejected_{0}, overflow_{0};
};
}}
