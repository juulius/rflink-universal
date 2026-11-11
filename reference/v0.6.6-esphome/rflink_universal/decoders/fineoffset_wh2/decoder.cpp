// Fine Offset WH2 (48-bit 0xFF preamble + 40-bit data) RX decoder.
// Protocol facts: rtl_433 src/devices/fineoffset.c; restrict to WH2, not WH2A/WH5.
#include "esphome/components/rflink_universal/decoder_registry.h"
#include "esphome/components/rflink_universal/format_utils.h"
#include "esphome/components/rflink_universal/pulse_tools.h"
#include <array>
#include <cstdint>

namespace esphome { namespace rflink_universal {
namespace {
using Bytes = std::array<uint8_t, 5>;
uint8_t crc8(const uint8_t *data, size_t size) {
  uint8_t crc = 0;
  for (size_t i = 0; i < size; ++i) {
    crc ^= data[i];
    for (unsigned bit = 0; bit < 8; ++bit) crc = (crc & 0x80u) ? static_cast<uint8_t>((crc << 1u) ^ 0x31u) : static_cast<uint8_t>(crc << 1u);
  }
  return crc;
}
bool read_bits(const std::vector<int32_t> &raw, size_t start, bool long_is_one, Bytes &b) {
  if (start + 96 > raw.size()) return false;
  uint8_t preamble = 0;
  b.fill(0);
  for (size_t bit = 0; bit < 48; ++bit) {
    const int32_t mark = raw[start + bit * 2];
    const int32_t gap = raw[start + bit * 2 + 1];
    if (mark <= 0 || gap >= 0 || !pulse::near_us(gap, 1000, 40)) return false;
    bool long_mark;
    if (pulse::near_us(mark, 1500, 35)) long_mark = true;
    else if (pulse::near_us(mark, 500, 35)) long_mark = false;
    else return false;
    const bool one = (long_mark == long_is_one);
    if (bit < 8) preamble = static_cast<uint8_t>((preamble << 1u) | unsigned(one));
    else if (one) b[(bit - 8) / 8] |= static_cast<uint8_t>(0x80u >> ((bit - 8) % 8));
  }
  if (preamble != 0xff || (b[0] >> 4u) != 4u || b[3] > 100u || b[0] == 0) return false;
  const int magnitude = ((b[1] & 0x0fu) << 8) | b[2];
  const int tenths = (magnitude & 0x800) ? -(magnitude & 0x7ff) : magnitude;
  return tenths >= -400 && tenths <= 700 && crc8(b.data(), 4) == b[4];
}
bool locate(const RFFrame &frame, Bytes &b) {
  if (frame.kind != FrameKind::RAW || !frame.raw || frame.frequency_hz < 433800000 || frame.frequency_hz > 434050000) return false;
  const auto &raw = *frame.raw;
  if (raw.size() < 96) return false;
  for (size_t start = 0; start + 96 <= raw.size() && start < 300; ++start) {
    if (read_bits(raw, start, true, b) || read_bits(raw, start, false, b)) return true;
  }
  return false;
}
}  // namespace
class FineOffsetWH2Decoder : public RFDecoder {
 public:
  const char *name() const override { return "FineOffsetWH2"; }
  uint8_t match(const RFFrame &f) const override { Bytes b{}; return locate(f, b) ? 95 : 0; }
  bool decode(const RFFrame &f, DecodeResult &r) const override {
    Bytes b{};
    if (!locate(f, b)) return false;
    const int raw_temp = ((b[1] & 0x0f) << 8) | b[2];
    const int tenths = (raw_temp & 0x800) ? -(raw_temp & 0x7ff) : raw_temp;
    const unsigned id = ((b[0] & 0x0f) << 4) | (b[1] >> 4);
    r = DecodeResult{};
    r.valid = true;
    r.protocol = "FineOffsetWH2";
    r.identity_key = hex_id("fineoffset_wh2", id, 2);
    r.identity_confidence = IdentityConfidence::MEDIUM;  // Random ID on start.
    r.device_type = "weather_sensor";
    r.action = "measurement";
    r.fields.push_back({"temperature", number_string(tenths / 10.0), "°C"});
    r.fields.push_back({"humidity", std::to_string(b[3]), "%"});
    r.fields.push_back({"crc", "valid", ""});
    return true;
  }
};
RFLINK_REGISTER_DECODER(FineOffsetWH2Decoder);
}}
