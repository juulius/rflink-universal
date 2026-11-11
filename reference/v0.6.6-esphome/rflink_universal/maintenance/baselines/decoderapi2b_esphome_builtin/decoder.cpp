#include "esphome/components/rflink_universal/decoder_registry.h"
#include "esphome/components/rflink_universal/format_utils.h"

#include "esphome/components/remote_base/byronsx_protocol.h"
#include "esphome/components/remote_base/dooya_protocol.h"
#include "esphome/components/remote_base/keeloq_protocol.h"
#include "esphome/components/remote_base/nexa_protocol.h"
#include "esphome/components/remote_base/rc_switch_protocol.h"

namespace esphome {
namespace rflink_universal {

using remote_base::RemoteReceiveData;
using remote_base::TOLERANCE_MODE_PERCENTAGE;

static RemoteReceiveData receive_data_(const RFFrame &frame) {
  return RemoteReceiveData(*frame.raw, 25, TOLERANCE_MODE_PERCENTAGE);
}

class NexaAdapter : public RFDecoder {
 public:
  const char *name() const override { return "Nexa"; }
  uint8_t match(const RFFrame &frame) const override {
    if (frame.kind != FrameKind::RAW || frame.raw == nullptr) return 0;
    auto src = receive_data_(frame);
    auto decoded = remote_base::NexaProtocol().decode(src);
    return decoded.has_value() ? 94 : 0;
  }
  bool decode(const RFFrame &frame, DecodeResult &result) const override {
    if (frame.raw == nullptr) return false;
    auto src = receive_data_(frame);
    auto decoded = remote_base::NexaProtocol().decode(src);
    if (!decoded.has_value()) return false;
    const auto &d = decoded.value();
    result.valid = true;
    result.protocol = "Nexa";
    result.identity_key = hex_id("nexa", d.device, 7);
    result.identity_confidence = IdentityConfidence::HIGH;
    result.device_type = "switch_remote";
    result.action = d.state ? "on" : "off";
    result.fields.push_back({"channel", std::to_string(d.channel), ""});
    result.fields.push_back({"group", d.group ? "true" : "false", ""});
    result.fields.push_back({"level", std::to_string(d.level), ""});
    result.fields.push_back({"tx_radio", std::to_string(frame.channel), ""});
    result.transmit_supported = true;
    result.expected_actions = {"on", "off"};
    return true;
  }
};
RFLINK_REGISTER_DECODER(NexaAdapter);

class ByronSXAdapter : public RFDecoder {
 public:
  const char *name() const override { return "ByronSX"; }
  uint8_t match(const RFFrame &frame) const override {
    if (frame.kind != FrameKind::RAW || frame.raw == nullptr) return 0;
    auto src = receive_data_(frame);
    return remote_base::ByronSXProtocol().decode(src).has_value() ? 92 : 0;
  }
  bool decode(const RFFrame &frame, DecodeResult &result) const override {
    auto src = receive_data_(frame);
    auto decoded = remote_base::ByronSXProtocol().decode(src);
    if (!decoded.has_value()) return false;
    const auto &d = decoded.value();
    result.valid = true;
    result.protocol = "ByronSX";
    result.identity_key = hex_id("byronsx", d.address, 2);
    result.identity_confidence = IdentityConfidence::HIGH;
    result.device_type = "doorbell_remote";
    result.action = "command_" + std::to_string(d.command);
    result.fields.push_back({"tx_radio", std::to_string(frame.channel), ""});
    result.transmit_supported = true;
    return true;
  }
};
RFLINK_REGISTER_DECODER(ByronSXAdapter);

class DooyaAdapter : public RFDecoder {
 public:
  const char *name() const override { return "Dooya"; }
  uint8_t match(const RFFrame &frame) const override {
    if (frame.kind != FrameKind::RAW || frame.raw == nullptr) return 0;
    auto src = receive_data_(frame);
    return remote_base::DooyaProtocol().decode(src).has_value() ? 93 : 0;
  }
  bool decode(const RFFrame &frame, DecodeResult &result) const override {
    auto src = receive_data_(frame);
    auto decoded = remote_base::DooyaProtocol().decode(src);
    if (!decoded.has_value()) return false;
    const auto &d = decoded.value();
    result.valid = true;
    result.protocol = "Dooya";
    result.identity_key = hex_id("dooya", d.id, 6);
    result.identity_confidence = IdentityConfidence::HIGH;
    result.device_type = "cover_remote";
    result.action = "button_" + std::to_string(d.button);
    result.fields.push_back({"channel", std::to_string(d.channel), ""});
    result.fields.push_back({"check", std::to_string(d.check), ""});
    // RX is proven by ESPHome's decoder. We keep TX hidden in RC3 until it is
    // checked on the target hardware/device variant.
    result.transmit_supported = false;
    return true;
  }
};
RFLINK_REGISTER_DECODER(DooyaAdapter);

class KeeloqAdapter : public RFDecoder {
 public:
  const char *name() const override { return "KeeLoq"; }
  uint8_t match(const RFFrame &frame) const override {
    if (frame.kind != FrameKind::RAW || frame.raw == nullptr) return 0;
    auto src = receive_data_(frame);
    return remote_base::KeeloqProtocol().decode(src).has_value() ? 94 : 0;
  }
  bool decode(const RFFrame &frame, DecodeResult &result) const override {
    auto src = receive_data_(frame);
    auto decoded = remote_base::KeeloqProtocol().decode(src);
    if (!decoded.has_value()) return false;
    const auto &d = decoded.value();
    result.valid = true;
    result.protocol = "KeeLoq";
    result.identity_key = hex_id("keeloq", d.address, 7);
    result.identity_confidence = IdentityConfidence::HIGH;
    result.device_type = "remote";
    result.action = "button_" + std::to_string(d.command);
    result.fields.push_back({"battery_low", d.vlow ? "true" : "false", ""});
    result.fields.push_back({"repeat", d.repeat ? "true" : "false", ""});
    // encrypted is deliberately NOT identity: it changes/hops.
    return true;
  }
};
RFLINK_REGISTER_DECODER(KeeloqAdapter);

class RCSwitchAdapter : public RFDecoder {
 public:
  const char *name() const override { return "RCSwitch"; }
  uint8_t match(const RFFrame &frame) const override {
    if (frame.kind != FrameKind::RAW || frame.raw == nullptr) return 0;
    for (uint8_t i = 1; i <= 8; i++) {
      auto src = receive_data_(frame);
      if (remote_base::RC_SWITCH_PROTOCOLS[i].decode(src).has_value()) return 65;
    }
    return 0;
  }
  bool decode(const RFFrame &frame, DecodeResult &result) const override {
    if (frame.raw == nullptr) return false;
    for (uint8_t i = 1; i <= 8; i++) {
      auto src = receive_data_(frame);
      auto decoded = remote_base::RC_SWITCH_PROTOCOLS[i].decode(src);
      if (!decoded.has_value()) continue;
      const auto &d = decoded.value();
      result.valid = true;
      result.protocol = "RCSwitch";
      result.identity_key = hex_id("rcswitch", d.code, 1);
      // Generic RC-Switch often cannot prove which bits are stable device ID
      // versus command/state, so it must never auto-merge.
      result.identity_confidence = IdentityConfidence::LOW;
      result.device_type = "generic_remote";
      result.action = "code";
      result.fields.push_back({"protocol", std::to_string(d.protocol), ""});
      result.fields.push_back({"code", std::to_string(static_cast<unsigned long long>(d.code)), ""});
      result.transmit_supported = true;
      return true;
    }
    return false;
  }
};
RFLINK_REGISTER_DECODER(RCSwitchAdapter);

}  // namespace rflink_universal
}  // namespace esphome
