#pragma once

#include <array>
#include <cstdint>
#include <vector>
#include <string>

namespace esphome {
namespace rflink_universal {
namespace somfy {

enum class Command : uint8_t {
  MY = 0x1,
  UP = 0x2,
  MY_UP = 0x3,
  DOWN = 0x4,
  MY_DOWN = 0x5,
  UP_DOWN = 0x6,
  MY_UP_DOWN = 0x7,
  PROG = 0x8,
  SUN_FLAG = 0x9,
  FLAG = 0xA,
};

struct Frame56 {
  uint8_t key{0};
  Command command{Command::MY};
  uint16_t rolling_code{0};
  uint32_t address{0};
};

std::array<uint8_t, 7> build_plain(const Frame56 &frame);
std::array<uint8_t, 7> obfuscate(const std::array<uint8_t, 7> &plain);
std::array<uint8_t, 7> deobfuscate(const std::array<uint8_t, 7> &wire);
bool checksum_valid(const std::array<uint8_t, 7> &plain);
bool decode_wire(const std::array<uint8_t, 7> &wire, Frame56 &out);
const char *command_name(Command command);
bool command_from_name(const std::string &name, Command &command);

// Build a complete Somfy RTS command waveform. The first frame contains the
// wake-up and two hardware-sync pulses; following frames use seven sync pulses.
// A single rolling code is reused across those RF repeats.
std::vector<int32_t> encode_raw_56(const Frame56 &frame, uint8_t frames = 3);

// Experimental raw 433.42 MHz RTS parser. It looks for the software sync,
// expands ~640 us Manchester half-bits, reconstructs 56 wire bits and then
// requires a valid Somfy checksum before accepting the frame.
bool decode_raw_56(const std::vector<int32_t> &raw, Frame56 &out);

}  // namespace somfy
}  // namespace rflink_universal
}  // namespace esphome
