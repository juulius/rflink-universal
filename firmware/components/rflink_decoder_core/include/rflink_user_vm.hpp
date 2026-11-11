#pragma once
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace rflink {

// Native, ESPHome-independent data contract for bounded user decoder recipes.
// These are normalized copies for interpreter use, never live CC1101/RMT views.
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
  const std::vector<int32_t> *raw{nullptr};
  const std::vector<uint8_t> *packet{nullptr};
};
struct DecodeResult {
  bool valid{false};
  std::string protocol;
  // Stable RF identity, not a command, sequence number, or rolling counter.
  std::string identity_key;
  IdentityConfidence identity_confidence{IdentityConfidence::LOW};
  std::string device_type;
  std::string action;
  std::vector<FieldValue> fields;
  bool transmit_supported{false};
};

namespace user_vm {
constexpr size_t MAX_SOURCE = 4096;
constexpr size_t MAX_INSTRUCTIONS = 192;
// The VM executes at most 2048 instructions with a 32-value stack. It cannot
// access GPIO, SPI, networking, storage, native functions or arbitrary memory.
enum class Mode : uint8_t { RX, TX };
struct Instruction {
  std::string op;
  std::string arg;
  std::string extra;
};
struct Program {
  std::vector<Instruction> code;
};
struct TxFrame {
  uint8_t radio{255};
  uint8_t repeats{1};
  std::vector<int32_t> raw;
  std::vector<uint8_t> packet;
  // Caller MUST persist any monotonic next_state safely before TX.
  // This VM has no state persistence, approval or RF transmit side effects.
  std::map<std::string, uint32_t> next_state;
};
bool identifier(const std::string &value);
bool compile(const std::string &source, Mode mode, Program &out, std::string &error);
bool receive(const Program &program, const std::string &protocol, const RFFrame &frame,
             DecodeResult &result, std::string &error);
bool transmit(const Program &program, const std::string &action, uint8_t radio,
              const std::map<std::string, uint32_t> &state, TxFrame &frame, std::string &error);
} // namespace user_vm
} // namespace rflink
