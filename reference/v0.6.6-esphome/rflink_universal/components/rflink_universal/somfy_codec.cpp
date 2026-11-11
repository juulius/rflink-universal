#include "somfy_codec.h"

#include <algorithm>
#include <cmath>

namespace esphome {
namespace rflink_universal {
namespace somfy {

static uint8_t checksum_nibble_(const std::array<uint8_t, 7> &frame) {
  uint8_t checksum = 0;
  for (auto byte : frame) checksum ^= byte ^ (byte >> 4u);
  return checksum & 0x0Fu;
}

std::array<uint8_t, 7> build_plain(const Frame56 &frame) {
  std::array<uint8_t, 7> data{};
  data[0] = frame.key;
  data[1] = static_cast<uint8_t>(static_cast<uint8_t>(frame.command) << 4u);
  data[2] = static_cast<uint8_t>(frame.rolling_code >> 8u);
  data[3] = static_cast<uint8_t>(frame.rolling_code & 0xFFu);
  data[4] = static_cast<uint8_t>(frame.address & 0xFFu);
  data[5] = static_cast<uint8_t>((frame.address >> 8u) & 0xFFu);
  data[6] = static_cast<uint8_t>((frame.address >> 16u) & 0xFFu);
  data[1] |= checksum_nibble_(data);
  return data;
}

std::array<uint8_t, 7> obfuscate(const std::array<uint8_t, 7> &plain) {
  auto data = plain;
  for (size_t i = 1; i < data.size(); i++) data[i] ^= data[i - 1];
  return data;
}

std::array<uint8_t, 7> deobfuscate(const std::array<uint8_t, 7> &wire) {
  auto data = wire;
  for (size_t i = data.size() - 1; i > 0; i--) data[i] ^= data[i - 1];
  return data;
}

bool checksum_valid(const std::array<uint8_t, 7> &plain) {
  return checksum_nibble_(plain) == 0;
}

bool decode_wire(const std::array<uint8_t, 7> &wire, Frame56 &out) {
  const auto plain = deobfuscate(wire);
  if (!checksum_valid(plain)) return false;
  out.key = plain[0];
  out.command = static_cast<Command>((plain[1] >> 4u) & 0x0Fu);
  out.rolling_code = static_cast<uint16_t>((static_cast<uint16_t>(plain[2]) << 8u) | plain[3]);
  out.address = static_cast<uint32_t>(plain[4]) |
                (static_cast<uint32_t>(plain[5]) << 8u) |
                (static_cast<uint32_t>(plain[6]) << 16u);
  return true;
}

const char *command_name(Command command) {
  switch (command) {
    case Command::MY: return "my";
    case Command::UP: return "up";
    case Command::MY_UP: return "my_up";
    case Command::DOWN: return "down";
    case Command::MY_DOWN: return "my_down";
    case Command::UP_DOWN: return "up_down";
    case Command::MY_UP_DOWN: return "my_up_down";
    case Command::PROG: return "prog";
    case Command::SUN_FLAG: return "sun_flag";
    case Command::FLAG: return "flag";
    default: return "unknown";
  }
}


bool command_from_name(const std::string &name, Command &command) {
  if (name == "my") command = Command::MY;
  else if (name == "up") command = Command::UP;
  else if (name == "down") command = Command::DOWN;
  else if (name == "prog") command = Command::PROG;
  else return false;
  return true;
}

std::vector<int32_t> encode_raw_56(const Frame56 &frame, uint8_t frames) {
  std::vector<int32_t> raw;
  if (frames == 0) return raw;
  frames = std::min<uint8_t>(frames, 12);
  const auto wire = obfuscate(build_plain(frame));
  auto level = [&](bool high, uint32_t us) {
    if (us == 0) return;
    const int32_t value = high ? static_cast<int32_t>(us) : -static_cast<int32_t>(us);
    if (!raw.empty() && ((raw.back() > 0) == high)) raw.back() += value;
    else raw.push_back(value);
  };
  auto one_frame = [&](uint8_t hardware_sync, bool wake) {
    if (wake) { level(true, 9415); level(false, 89565); }
    for (uint8_t i = 0; i < hardware_sync; ++i) { level(true, 2560); level(false, 2560); }
    level(true, 4550); level(false, 640);
    for (const auto byte : wire) {
      for (int bit = 7; bit >= 0; --bit) {
        const bool one = ((byte >> bit) & 1u) != 0;
        // Somfy RTS Manchester: 1 = low/high, 0 = high/low.
        level(!one, 640);
        level(one, 640);
      }
    }
    level(false, 30415);
  };
  one_frame(2, true);
  for (uint8_t i = 1; i < frames; ++i) one_frame(7, false);
  return raw;
}

static bool near_(uint32_t value, uint32_t target, uint8_t tolerance = 30) {
  return value >= target * (100u - tolerance) / 100u && value <= target * (100u + tolerance) / 100u;
}

bool decode_raw_56(const std::vector<int32_t> &raw, Frame56 &out) {
  if (raw.size() < 30) return false;

  // Find Somfy software sync: ~4550 us mark + ~640 us space. The sync
  // space can be merged by the receiver/transmitter with the first LOW
  // Manchester half-bit, producing ~1280 us. Preserve that extra half-bit.
  size_t start = raw.size();
  uint8_t leading_low_halves = 0;
  constexpr uint32_t HALF_US = 640;
  for (size_t i = 0; i + 1 < raw.size(); i++) {
    if (raw[i] <= 0 || raw[i + 1] >= 0 ||
        !near_(static_cast<uint32_t>(raw[i]), 4550, 35)) continue;
    const uint32_t low = static_cast<uint32_t>(-raw[i + 1]);
    if (near_(low, HALF_US, 45)) leading_low_halves = 0;
    else if (near_(low, HALF_US * 2u, 35)) leading_low_halves = 1;
    else continue;
    start = i + 2;
    break;
  }
  if (start == raw.size()) return false;

  // Convert durations to Manchester half-bit levels. Real captures merge
  // consecutive equal half-bits into 1x/2x/3x durations.
  std::vector<bool> halves;
  halves.reserve(120);
  if (leading_low_halves) halves.push_back(false);
  for (size_t i = start; i < raw.size() && halves.size() < 112; i++) {
    const uint32_t width = static_cast<uint32_t>(std::abs(raw[i]));
    uint32_t count = static_cast<uint32_t>(std::lround(static_cast<double>(width) / HALF_US));
    // If the final Manchester half is LOW, a demodulator may merge it with
    // the ~30.4 ms inter-frame gap. Recover that one last data half.
    if (count > 4 && halves.size() == 111 && raw[i] < 0 && near_(width, 30415u + HALF_US, 20)) {
      halves.push_back(false);
      break;
    }
    if (count < 1 || count > 4) return false;
    const uint32_t expected = count * HALF_US;
    if (!near_(width, expected, 35)) return false;
    const bool level = raw[i] > 0;
    for (uint32_t n = 0; n < count && halves.size() < 112; n++) halves.push_back(level);
  }
  if (halves.size() < 112) return false;

  // Manchester pair orientation can be inverted by receiver configuration.
  for (int inversion = 0; inversion < 2; inversion++) {
    std::array<uint8_t, 7> wire{};
    bool valid_pairs = true;
    for (size_t bit = 0; bit < 56; bit++) {
      bool a = halves[bit * 2];
      bool b = halves[bit * 2 + 1];
      if (inversion) { a = !a; b = !b; }
      if (a == b) { valid_pairs = false; break; }
      const bool value = a && !b;
      if (value) wire[bit / 8] |= static_cast<uint8_t>(0x80u >> (bit % 8));
    }
    if (valid_pairs && decode_wire(wire, out)) return true;
  }
  return false;
}

}  // namespace somfy
}  // namespace rflink_universal
}  // namespace esphome
