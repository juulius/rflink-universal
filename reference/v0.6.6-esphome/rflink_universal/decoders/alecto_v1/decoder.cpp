#include "esphome/components/rflink_universal/decoder_registry.h"
#include "esphome/components/rflink_universal/format_utils.h"
#include "esphome/components/rflink_universal/pulse_tools.h"

#include <array>
#include <cmath>

namespace esphome {
namespace rflink_universal {

namespace {

bool checksum_ok(const std::array<uint8_t, 5> &b) {
  uint8_t csum = 0;
  for (size_t i = 0; i < 4; i++) {
    const uint8_t tmp = pulse::reverse8(b[i]);
    csum = static_cast<uint8_t>(csum + (tmp & 0x0Fu) + (tmp >> 4u));
  }
  csum = ((b[1] & 0x7Fu) == 0x6Cu) ? static_cast<uint8_t>(csum + 0x07u)
                                     : static_cast<uint8_t>(0x0Fu - csum);
  csum = pulse::reverse8(static_cast<uint8_t>((csum & 0x0Fu) << 4u));
  return csum == (b[4] >> 4u);
}

bool bits_to_frame(const std::vector<int32_t> &raw, size_t start, bool invert, std::array<uint8_t, 5> &b) {
  b.fill(0);
  if (start + 72 > raw.size()) return false;
  for (size_t bit = 0; bit < 36; bit++) {
    const int32_t mark = raw[start + bit * 2];
    const int32_t gap = raw[start + bit * 2 + 1];
    if (!pulse::near_us(mark, 500, 45)) return false;
    const uint32_t width = static_cast<uint32_t>(std::abs(gap));
    bool one;
    if (pulse::near_us(width, 4000, 35)) one = true;
    else if (pulse::near_us(width, 2000, 35)) one = false;
    else return false;
    if (invert) one = !one;
    if (one) b[bit / 8] |= static_cast<uint8_t>(0x80u >> (bit % 8));
  }
  return checksum_ok(b);
}

bool find_frame(const std::vector<int32_t> &raw, std::array<uint8_t, 5> &b) {
  if (raw.size() < 72) return false;
  for (size_t start = 0; start + 72 <= raw.size(); start++) {
    if (bits_to_frame(raw, start, false, b) || bits_to_frame(raw, start, true, b)) return true;
  }
  return false;
}

}  // namespace

class AlectoV1Decoder : public RFDecoder {
 public:
  const char *name() const override { return "AlectoV1"; }

  uint8_t match(const RFFrame &frame) const override {
    if (frame.kind != FrameKind::RAW || frame.raw == nullptr) return 0;
    if (frame.frequency_hz < 433000000 || frame.frequency_hz > 434500000) return 0;
    std::array<uint8_t, 5> b{};
    return find_frame(*frame.raw, b) ? 92 : 0;
  }

  bool decode(const RFFrame &frame, DecodeResult &result) const override {
    if (frame.raw == nullptr) return false;
    std::array<uint8_t, 5> b{};
    if (!find_frame(*frame.raw, b)) return false;

    const uint8_t sensor_id = pulse::reverse8(b[0]);
    const uint8_t channel = static_cast<uint8_t>((b[0] & 0x0Cu) >> 2u);
    const bool battery_low = (b[1] & 0x80u) != 0;

    result.valid = true;
    result.protocol = "AlectoV1";
    result.identity_key = hex_id("alecto_v1", sensor_id, 2);
    // rtl_433 describes this as a random sensor ID; after a battery change it
    // may change, so RFLink must suggest rather than auto-merge.
    result.identity_confidence = IdentityConfidence::MEDIUM;
    result.device_type = "weather_sensor";
    result.fields.push_back({"channel", std::to_string(channel), ""});
    result.fields.push_back({"battery_low", battery_low ? "true" : "false", ""});

    if ((b[1] & 0x0Fu) == 0x0Cu) {
      const uint16_t rain_raw = static_cast<uint16_t>((pulse::reverse8(b[3]) << 8u) | pulse::reverse8(b[2]));
      result.action = "rain_update";
      result.fields.push_back({"rain", number_string(rain_raw * 0.25, 2), "mm"});
      return true;
    }

    const uint16_t temp_raw_u = static_cast<uint16_t>((pulse::reverse8(b[1]) & 0xF0u) |
                                                       (static_cast<uint16_t>(pulse::reverse8(b[2])) << 8u));
    int16_t temp_raw = static_cast<int16_t>(temp_raw_u);
    temp_raw = static_cast<int16_t>(temp_raw >> 4u);
    const double temp_c = temp_raw * 0.1;
    const uint8_t hum_bcd = pulse::reverse8(b[3]);
    const uint8_t humidity = static_cast<uint8_t>((hum_bcd >> 4u) * 10u + (hum_bcd & 0x0Fu));
    result.action = "weather_update";
    result.fields.push_back({"temperature", number_string(temp_c, 1), "°C"});
    if (humidity <= 100) result.fields.push_back({"humidity", std::to_string(humidity), "%"});
    return true;
  }
};

RFLINK_REGISTER_DECODER(AlectoV1Decoder);

}  // namespace rflink_universal
}  // namespace esphome
