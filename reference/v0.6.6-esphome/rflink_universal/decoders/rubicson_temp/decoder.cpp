// Rubicson / TFA 30.3197 / InFactory temperature RX. Protocol: rtl_433 rubicson.c.
#include "esphome/components/rflink_universal/decoder_registry.h"
#include "esphome/components/rflink_universal/format_utils.h"
#include "esphome/components/rflink_universal/pulse_tools.h"
#include <array>
#include <cstdint>

namespace esphome { namespace rflink_universal {
namespace {
using Bytes = std::array<uint8_t, 5>;
uint8_t crc8(const uint8_t *data, size_t size) {
  uint8_t crc = 0x6c;
  for (size_t i = 0; i < size; ++i) {
    crc ^= data[i];
    for (uint8_t k = 0; k < 8; ++k) crc = (crc & 0x80u) ? static_cast<uint8_t>((crc << 1) ^ 0x31u) : static_cast<uint8_t>(crc << 1);
  }
  return crc;
}
bool read_bits(const std::vector<int32_t> &raw, size_t start, Bytes &b) {
  if (start + 72 > raw.size()) return false;
  b.fill(0);
  for (size_t bit = 0; bit < 36; ++bit) {
    const int32_t mark = raw[start + bit * 2];
    const int32_t gap = raw[start + bit * 2 + 1];
    if (mark <= 0 || gap >= 0 || !pulse::near_us(mark, 500, 40)) return false;
    bool one;
    if (pulse::near_us(gap, 2000, 35)) one = true;
    else if (pulse::near_us(gap, 1000, 35)) one = false;
    else return false;
    if (one) b[bit / 8] |= static_cast<uint8_t>(0x80u >> (bit % 8));
  }
  if ((b[3] & 0xf0u) != 0xf0u || (b[1] & 0x40u)) return false;
  const uint8_t crc_bytes[5] = {b[0], b[1], b[2], static_cast<uint8_t>(b[3] & 0xf0u),
                               static_cast<uint8_t>((b[3] << 4u) | (b[4] >> 4u))};
  if (crc8(crc_bytes, 5) != 0) return false;
  const int16_t temp = static_cast<int16_t>((b[1] << 12) | (b[2] << 4)) >> 4;
  return b[0] != 0 && temp >= -400 && temp <= 700;
}
bool locate(const RFFrame &frame, Bytes &b) {
  if (frame.kind != FrameKind::RAW || !frame.raw || frame.frequency_hz < 433800000 || frame.frequency_hz > 434050000) return false;
  const auto &raw = *frame.raw;
  if (raw.size() < 72) return false;
  for (size_t start = 0; start + 72 <= raw.size() && start < 300; ++start) if (read_bits(raw, start, b)) return true;
  return false;
}
}  // namespace
class RubicsonTempDecoder : public RFDecoder {
 public:
  const char *name() const override { return "RubicsonTemp"; }
  uint8_t match(const RFFrame &f) const override { Bytes b{}; return locate(f, b) ? 95 : 0; }
  bool decode(const RFFrame &f, DecodeResult &r) const override {
    Bytes b{};
    if (!locate(f, b)) return false;
    const int16_t tenths = static_cast<int16_t>((b[1] << 12) | (b[2] << 4)) >> 4;
    r = DecodeResult{};
    r.valid = true;
    r.protocol = "RubicsonTemp";
    // The channel is stable for the sensor, and distinguishes identical IDs
    // on different channels. Power cycling may still change the random ID.
    r.identity_key = hex_id("rubicson", (unsigned(b[0]) << 2u) | ((b[1] >> 4u) & 0x03u), 3);
    r.identity_confidence = IdentityConfidence::MEDIUM;
    r.device_type = "temperature_sensor";
    r.action = "measurement";
    r.fields.push_back({"temperature", number_string(tenths / 10.0), "°C"});
    r.fields.push_back({"channel", std::to_string(((b[1] >> 4u) & 3u) + 1u), ""});
    r.fields.push_back({"battery_low", (b[1] & 0x80u) ? "false" : "true", ""});
    r.fields.push_back({"crc", "valid", ""});
    return true;
  }
};
RFLINK_REGISTER_DECODER(RubicsonTempDecoder);
}}
