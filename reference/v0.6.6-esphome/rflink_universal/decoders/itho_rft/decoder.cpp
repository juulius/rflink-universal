#include "esphome/components/rflink_universal/decoder_registry.h"
#include "esphome/components/rflink_universal/format_utils.h"
#include "esphome/components/rflink_universal/itho_codec.h"

namespace esphome {
namespace rflink_universal {

class IthoRFTDecoder : public RFDecoder {
 public:
  const char *name() const override { return "IthoRFT"; }

  uint8_t match(const RFFrame &frame) const override {
    if (frame.kind != FrameKind::PACKET || frame.packet == nullptr) return 0;
    if (frame.frequency_hz < 868200000 || frame.frequency_hz > 868400000) return 0;
    itho::Decoded decoded;
    return itho::decode_packet(*frame.packet, decoded) ? 98 : 0;
  }

  bool decode(const RFFrame &frame, DecodeResult &result) const override {
    if (frame.packet == nullptr) return false;
    itho::Decoded decoded;
    if (!itho::decode_packet(*frame.packet, decoded)) return false;
    const uint32_t id = (static_cast<uint32_t>(decoded.source_id[0]) << 16u) |
                        (static_cast<uint32_t>(decoded.source_id[1]) << 8u) |
                        decoded.source_id[2];
    result.valid = true;
    result.protocol = "IthoRFT";
    result.identity_key = hex_id("itho_rft", id, 6);
    result.identity_confidence = IdentityConfidence::HIGH;
    result.device_type = "ventilation";
    result.pairing_supported = true;
    result.transmit_supported = true;
    result.expected_actions = {"standby", "low", "medium", "high", "timer_10", "timer_20", "timer_30"};
    switch (decoded.type) {
      case itho::MessageType::SPEED:
        result.action = decoded.speed == 1 ? "standby" : decoded.speed == 2 ? "low" : decoded.speed == 3 ? "medium" : "high";
        break;
      case itho::MessageType::TIMER:
        result.action = "timer_" + std::to_string(decoded.timer_minutes);
        break;
      case itho::MessageType::UNIT_SPEED:
        result.action = "status";
        result.fields.push_back({"fan_speed", std::to_string(decoded.fan_percent), "%"});
        break;
      case itho::MessageType::HUMIDITY:
        result.action = "status";
        result.fields.push_back({"humidity", std::to_string(decoded.humidity_percent), "%"});
        break;
      default: return false;
    }
    result.fields.push_back({"counter", std::to_string(decoded.counter), ""});
    return true;
  }
};

RFLINK_REGISTER_DECODER(IthoRFTDecoder);

}  // namespace rflink_universal
}  // namespace esphome
