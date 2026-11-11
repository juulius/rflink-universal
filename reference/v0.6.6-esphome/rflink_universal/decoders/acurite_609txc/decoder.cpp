// Acurite 609TXC RX decoder. Protocol facts: rtl_433 src/devices/acurite.c.
// Original implementation for RFLink's signed microsecond pulse interface.
#include "esphome/components/rflink_universal/decoder_registry.h"
#include "esphome/components/rflink_universal/format_utils.h"
#include "esphome/components/rflink_universal/pulse_tools.h"
#include <array>
#include <cstdint>

namespace esphome { namespace rflink_universal {
namespace {
using Bytes = std::array<uint8_t, 5>;

// One OOK PWM data bit = a short or long positive mark + negative gap.
// Reject pairs with inverted polarity or an implausible gap rather than
// interpreting arbitrary noise as a known sensor.
bool read_bits(const std::vector<int32_t> &raw, size_t start, bool long_is_one, Bytes &bytes) {
  if (start + 80 > raw.size()) return false;
  bytes.fill(0);
  for (size_t bit = 0; bit < 40; ++bit) {
    const int32_t mark = raw[start + bit * 2];
    const int32_t gap = raw[start + bit * 2 + 1];
    if (mark <= 0 || gap >= 0 || !pulse::near_us(gap, 1000, 40)) return false;
    bool long_mark;
    if (pulse::near_us(mark, 1000, 32)) long_mark = true;
    else if (pulse::near_us(mark, 500, 32)) long_mark = false;
    else return false;
    const bool one = long_is_one ? long_mark : !long_mark;
    if (one) bytes[bit / 8] |= static_cast<uint8_t>(0x80u >> (bit % 8));
  }
  const unsigned sum = unsigned(bytes[0]) + bytes[1] + bytes[2] + bytes[3];
  if (sum == 0 || static_cast<uint8_t>(sum) != bytes[4]) return false;
  const int16_t tenths = static_cast<int16_t>(((bytes[1] & 0x0Fu) << 12) | (bytes[2] << 4)) >> 4;
  return bytes[0] != 0 && bytes[3] <= 100 && tenths >= -400 && tenths <= 700;
}
bool locate(const RFFrame &frame, Bytes &bytes) {
  if (frame.kind != FrameKind::RAW || !frame.raw || frame.frequency_hz < 433800000 || frame.frequency_hz > 434050000) return false;
  const auto &raw = *frame.raw;
  if (raw.size() < 80) return false;
  // rtl_433 demodulates the long-mark vs short-mark polarity for this model.
  // Allow either phase; the additive checksum and numeric bounds must agree.
  for (size_t start = 0; start + 80 <= raw.size() && start < 300; ++start) {
    if (read_bits(raw, start, true, bytes) || read_bits(raw, start, false, bytes)) return true;
  }
  return false;
}
}  // namespace
class Acurite609TXCDecoder : public RFDecoder {
 public:
  const char *name() const override { return "Acurite609TXC"; }
  uint8_t match(const RFFrame &frame) const override { Bytes b{}; return locate(frame, b) ? 88 : 0; }
  bool decode(const RFFrame &frame, DecodeResult &result) const override {
    Bytes b{};
    if (!locate(frame, b)) return false;
    const int16_t tenths = static_cast<int16_t>(((b[1] & 0x0Fu) << 12) | (b[2] << 4)) >> 4;
    result = DecodeResult{};
    result.valid = true;
    result.protocol = "Acurite609TXC";
    result.identity_key = hex_id("acurite609", b[0], 2);
    result.identity_confidence = IdentityConfidence::MEDIUM;  // ID changes on power cycle.
    result.device_type = "weather_sensor";
    result.action = "measurement";
    result.fields.push_back({"temperature", number_string(tenths / 10.0), "°C"});
    result.fields.push_back({"humidity", std::to_string(b[3]), "%"});
    result.fields.push_back({"battery_low", (b[1] & 0x80) ? "true" : "false", ""});
    result.fields.push_back({"checksum", "valid", ""});
    return true;
  }
};
RFLINK_REGISTER_DECODER(Acurite609TXCDecoder);
}}  // namespace
