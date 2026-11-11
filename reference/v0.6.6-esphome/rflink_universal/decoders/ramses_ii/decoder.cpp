// Built-in RFLink Universal decoder for RAMSES-II, including Orcon 15RF devices.
// Packet-only RX; does not reconfigure the CC1101 and does not support TX.
#include "esphome/components/rflink_universal/decoder_registry.h"
#include "esphome/components/rflink_universal/format_utils.h"
#include "codec.h"
#include <cstdio>
#include <string>

namespace esphome {
namespace rflink_universal {
namespace {

std::string ramses_identity(uint32_t wire) {
  char buf[36];
  // Decimal RAMSES 6-bit device class + 18-bit serial.
  std::snprintf(buf, sizeof(buf), "ramses:%02u:%06lu",
                unsigned(rflink_ramses_ii::deviceClass(wire)),
                static_cast<unsigned long>(rflink_ramses_ii::deviceSerial(wire)));
  return buf;
}

std::string payload_hex(const rflink_ramses_ii::Frame &message) {
  static const char hex[] = "0123456789ABCDEF";
  std::string out;
  out.reserve(static_cast<size_t>(message.payload_length) * 2);
  for (unsigned i = 0; i < message.payload_length; ++i) {
    out.push_back(hex[message.payload[i] >> 4u]);
    out.push_back(hex[message.payload[i] & 15u]);
  }
  return out;
}

const char *frame_type(rflink_ramses_ii::Frame::Type type) {
  switch (type) {
    case rflink_ramses_ii::Frame::INFORMATION: return "information";
    case rflink_ramses_ii::Frame::REQUEST: return "request";
    case rflink_ramses_ii::Frame::RESPONSE: return "response";
    case rflink_ramses_ii::Frame::WRITE: return "write";
    default: return "unknown";
  }
}

const char *mode_key(uint8_t mode) {
  switch (mode) {
    case 0: return "away";
    case 1: return "low";
    case 2: return "medium";
    case 3: return "high";
    case 4: return "auto";
    case 5: return "auto_2";
    case 6: return "boost";
    case 7: return "disabled";
    default: return "unknown";
  }
}
} // namespace

class RamsesIIDecoder : public RFDecoder {
 public:
  const char *name() const override { return "RamsesII"; }

  uint8_t match(const RFFrame &frame) const override {
    if (frame.kind != FrameKind::PACKET || frame.packet == nullptr ||
        frame.frequency_hz < 868200000 || frame.frequency_hz > 868400000) return 0;
    rflink_ramses_ii::Frame parsed;
    // Full sync + Manchester symbols + a validated checksum/length. Score >
    // legacy Itho match so RAMSES frames cannot be mislabeled as Itho.
    return rflink_ramses_ii::decodePacked(frame.packet->data(), frame.packet->size(), parsed) ? 99 : 0;
  }

  bool decode(const RFFrame &frame, DecodeResult &result) const override {
    if (frame.kind != FrameKind::PACKET || frame.packet == nullptr ||
        frame.frequency_hz < 868200000 || frame.frequency_hz > 868400000) return false;
    rflink_ramses_ii::Frame parsed;
    if (!rflink_ramses_ii::decodePacked(frame.packet->data(), frame.packet->size(), parsed)) return false;
    // RAMSES frame addresses are not necessarily a physical remote identity.
    // An explicit source (address index 0) is needed for stable device learning.
    if (!parsed.has_addr[0]) return false;
    result = DecodeResult{};
    result.valid = true;
    result.protocol = "RamsesII";
    result.identity_key = ramses_identity(parsed.addr[0]);
    result.identity_confidence = IdentityConfidence::HIGH;
    result.device_type = "ventilation";
    result.pairing_supported = false; // RX 1FC9 does not make TX pairing available.
    result.transmit_supported = false;
    result.expected_actions = {"status", "mode_command", "timer_command", "pairing_event", "measurement", "message"};
    char cmd[5];
    std::snprintf(cmd, sizeof(cmd), "%04X", unsigned(parsed.command));
    result.fields.push_back({"ramses_command", cmd, ""});
    result.fields.push_back({"ramses_frame_type", frame_type(parsed.type), ""});
    result.fields.push_back({"ramses_payload", payload_hex(parsed), ""});
    result.fields.push_back({"ramses_payload_length", std::to_string(parsed.payload_length), ""});
    switch (parsed.command) {
      case 0x31D9: result.action = "status"; break;
      case 0x31DA: result.action = "status"; break;
      case 0x22F1: result.action = "mode_command"; break;
      case 0x22F3: result.action = "timer_command"; break;
      case 0x1FC9: result.action = "pairing_event"; break;
      case 0x1298: case 0x12A0: result.action = "measurement"; break;
      default: result.action = "message"; break;
    }
    uint8_t mode = 0;
    if (rflink_ramses_ii::extractMode(parsed, mode)) {
      result.fields.push_back({"fan_mode", mode_key(mode), ""});
      result.fields.push_back({"fan_mode_code", std::to_string(mode), ""});
    }
    return true;
  }
};

RFLINK_REGISTER_DECODER(RamsesIIDecoder);

} // namespace rflink_universal
} // namespace esphome
