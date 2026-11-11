/*
 * BEGINNER EXAMPLE DECODER / VOORBEELDDECODER
 *
 * A pulse / puls is one period in which the received signal is HIGH or LOW.
 * Pulse width / pulslengte is its duration in microseconds (us).
 * A frame / telegram is one complete RF message.
 * A bit is 0 or 1; one bit often consists of two pulses.
 *
 * Copy this file, rename the class, and keep the RFLINK_REGISTER_DECODER line.
 * You do NOT edit a central constants file or decoder list.
 */
#include "esphome/components/rflink_universal/decoder_registry.h"
#include "esphome/components/rflink_universal/pulse_tools.h"

namespace esphome {
namespace rflink_universal {

class ExampleSwitchDecoder : public RFDecoder {
 public:
  const char *name() const override { return "ExampleSwitch"; }

  uint8_t match(const RFFrame &frame) const override {
    // Safety: example never claims a real signal.
    // Typical future checks:
    // if (frame.frequency_hz != 433920000) return 0;
    // if (frame.raw == nullptr || frame.raw->size() != 50) return 0;
    (void) frame;
    return 0;
  }

  bool decode(const RFFrame &frame, DecodeResult &result) const override {
    if (frame.raw == nullptr) return false;

    // Example helper usage:
    // pulse::Timing t{350, 1050, 25};
    // std::vector<bool> bits;
    // if (!pulse::decode_pairs(*frame.raw, 2, 24, t,
    //        pulse::PairEncoding::SHORT_LONG_IS_ZERO, bits)) return false;
    // auto transmitter = pulse::bits_to_uint(bits, 0, 16);
    // auto command = pulse::bits_to_uint(bits, 16, 4);

    result.valid = false;
    result.protocol = "ExampleSwitch";
    result.device_type = "remote";
    return false;
  }
};

RFLINK_REGISTER_DECODER(ExampleSwitchDecoder);

}  // namespace rflink_universal
}  // namespace esphome
