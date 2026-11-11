#include "esphome/components/rflink_universal/decoder_registry.h"
#include "esphome/components/rflink_universal/format_utils.h"
#include "esphome/components/rflink_universal/somfy_codec.h"

namespace esphome {
namespace rflink_universal {

class SomfyRTS56Decoder : public RFDecoder {
 public:
  const char *name() const override { return "SomfyRTS56"; }

  uint8_t match(const RFFrame &frame) const override {
    if (frame.kind != FrameKind::RAW || frame.raw == nullptr) return 0;
    if (frame.frequency_hz < 433300000 || frame.frequency_hz > 433600000) return 0;
    somfy::Frame56 decoded;
    return somfy::decode_raw_56(*frame.raw, decoded) ? 96 : 0;
  }

  bool decode(const RFFrame &frame, DecodeResult &result) const override {
    if (frame.raw == nullptr) return false;
    somfy::Frame56 decoded;
    if (!somfy::decode_raw_56(*frame.raw, decoded)) return false;
    result.valid = true;
    result.protocol = "SomfyRTS56";
    result.identity_key = hex_id("somfy_rts", decoded.address, 6);
    result.identity_confidence = IdentityConfidence::HIGH;
    result.device_type = "cover_remote";
    result.action = somfy::command_name(decoded.command);
    result.fields.push_back({"rolling_code", std::to_string(decoded.rolling_code), ""});
    result.pairing_supported = true;
    // Native TX uses a separate virtual RFLink remote identity. Physical motor
    // validation is still pending, but the rolling-code/pairing path is present.
    result.transmit_supported = true;
    result.expected_actions = {"up", "down", "my", "prog"};
    return true;
  }
};

RFLINK_REGISTER_DECODER(SomfyRTS56Decoder);

}  // namespace rflink_universal
}  // namespace esphome
