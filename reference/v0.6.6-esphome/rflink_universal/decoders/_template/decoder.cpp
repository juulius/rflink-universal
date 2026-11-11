/*
 * RFLink Universal beginner decoder template.
 *
 * Copying is normally done with:
 *   python tools/new_decoder.py my_protocol "My Protocol"
 *
 * Important rule: this file should only translate RFFrame -> DecodeResult.
 * Do not control the web UI, Home Assistant, GPIO or another decoder here.
 */
#include "esphome/components/rflink_universal/decoder_registry.h"
#include "esphome/components/rflink_universal/format_utils.h"
#include "esphome/components/rflink_universal/pulse_tools.h"

namespace esphome {
namespace rflink_universal {

class __CLASS_NAME__Decoder : public RFDecoder {
 public:
  const char *name() const override { return "__DISPLAY_NAME__"; }

  uint8_t match(const RFFrame &frame) const override {
    // Step 1: choose the correct kind/frequency.
    if (frame.kind != FrameKind::RAW || frame.raw == nullptr) return 0;
    if (frame.frequency_hz < 433000000 || frame.frequency_hz > 434500000) return 0;

    // Step 2: add checks that make this protocol distinctive.
    // Examples: pulse count, sync pulse, short/long timings, checksum.
    // Never return a high score using only frequency or pulse count.
    return 0;  // Safe until you implement recognition.
  }

  bool decode(const RFFrame &frame, DecodeResult &result) const override {
    if (this->match(frame) == 0) return false;

    // Step 3: decode the stable physical-device identity.
    // Do NOT include button state, rolling counter or checksum in identity_key.
    result.valid = true;
    result.protocol = "__CLASS_NAME__";
    result.identity_key = "__PACK_ID__:TODO";
    result.identity_confidence = IdentityConfidence::LOW;

    // Step 4: describe what the device is and what happened.
    result.device_type = "remote";
    result.action = "unknown";

    // Optional measured/state values:
    // result.fields.push_back({"battery_low", "false", ""});

    // Optional capabilities known from the protocol:
    // result.expected_actions = {"on", "off"};
    // result.pairing_supported = true;
    // result.transmit_supported = true;
    return true;
  }
};

RFLINK_REGISTER_DECODER(__CLASS_NAME__Decoder);

}  // namespace rflink_universal
}  // namespace esphome
