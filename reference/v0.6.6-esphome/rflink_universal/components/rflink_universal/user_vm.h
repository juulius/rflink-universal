#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "decoder.h"

namespace esphome {
namespace rflink_universal {
namespace user_vm {
constexpr size_t MAX_SOURCE = 4096;
constexpr size_t MAX_INSTRUCTIONS = 192;

// A deliberately small, deterministic instruction set. User programs can
// inspect a received frame or construct a TX frame, but have no memory access,
// GPIO, networking, filesystem access or native-code escape hatch.
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
  std::map<std::string, uint32_t> next_state;
};

bool identifier(const std::string &value);
bool compile(const std::string &source, Mode mode, Program &out, std::string &error);
bool receive(const Program &program, const std::string &protocol, const RFFrame &frame,
             DecodeResult &result, std::string &error);
bool transmit(const Program &program, const std::string &action, uint8_t radio,
              const std::map<std::string, uint32_t> &state, TxFrame &frame, std::string &error);

}  // namespace user_vm
}  // namespace rflink_universal
}  // namespace esphome
