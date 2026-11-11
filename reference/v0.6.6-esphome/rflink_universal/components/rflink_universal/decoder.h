#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace esphome {
namespace rflink_universal {

enum class FrameKind : uint8_t { RAW = 0, PACKET = 1 };
enum class IdentityConfidence : uint8_t { LOW = 0, MEDIUM = 1, HIGH = 2 };

struct FieldValue {
  std::string key;
  std::string value;
  std::string unit;
};

struct RFFrame {
  FrameKind kind{FrameKind::RAW};
  uint8_t channel{0};
  uint32_t frequency_hz{0};
  uint32_t received_ms{0};
  float rssi_dbm{0.0f};
  bool has_rssi{false};
  uint8_t lqi{0};
  bool has_lqi{false};
  float frequency_offset_hz{0.0f};
  const std::vector<int32_t> *raw{nullptr};
  const std::vector<uint8_t> *packet{nullptr};
};

struct DecodeResult {
  bool valid{false};
  std::string protocol;

  // The decoder-provided stable RF identity. Do NOT include command bits,
  // rolling counters or button state here.
  std::string identity_key;
  IdentityConfidence identity_confidence{IdentityConfidence::LOW};

  std::string device_type;
  std::string action;
  std::vector<FieldValue> fields;

  bool pairing_supported{false};
  bool transmit_supported{false};
  std::vector<std::string> expected_actions;
};

class RFDecoder {
 public:
  virtual ~RFDecoder() = default;
  virtual const char *name() const = 0;
  virtual uint8_t match(const RFFrame &frame) const = 0;
  virtual bool decode(const RFFrame &frame, DecodeResult &result) const = 0;
};

inline const char *identity_confidence_name(IdentityConfidence confidence) {
  switch (confidence) {
    case IdentityConfidence::HIGH: return "HIGH";
    case IdentityConfidence::MEDIUM: return "MEDIUM";
    default: return "LOW";
  }
}

}  // namespace rflink_universal
}  // namespace esphome
