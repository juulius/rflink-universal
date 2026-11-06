#pragma once
// Synthetic protocol test frames. No captured or personally paired RF device data.
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>
#include "rflink_ramses_codec.hpp"
#include "rflink_ramses_tx.hpp"
namespace synthetic_ramses {
inline std::array<uint8_t,63> frame(uint16_t command) {
  rflink_ramses_ii::Frame msg;
  msg.type = rflink_ramses_ii::Frame::INFORMATION;
  const uint32_t dummy_id = (37u<<18) | 0x1234u;
  msg.has_addr[0] = true;
  msg.has_addr[2] = true;
  msg.addr[0] = dummy_id;
  msg.addr[2] = dummy_id;
  msg.command = command;
  msg.payload_length = 3;
  msg.payload[0] = 0;
  msg.payload[1] = 2;
  msg.payload[2] = 4;
  std::vector<uint8_t> wire;
  assert(rflink_ramses_ii::tx::encodeWire(msg, wire));
  assert(wire.size() >= 6 && wire[2] == 0x33 && wire[3] == 0x55 && wire[4] == 0x53);
  // Synthetic CC1101-style packet preamble, followed by 8N1 UART wire bytes.
  std::array<uint8_t,63> packed{};
  const uint8_t prefix[] = {0x00,0xB3,0x2A,0xAB,0x2A};
  for (size_t i = 0; i < sizeof(prefix); ++i) packed[i] = prefix[i];
  size_t bit_position = sizeof(prefix) * 8;
  auto write_bit = [&](bool value) {
    assert(bit_position < packed.size()*8);
    if (value) packed[bit_position / 8] |= static_cast<uint8_t>(0x80u >> (bit_position % 8));
    ++bit_position;
  };
  for (size_t i = 2; i < wire.size(); ++i) {
    write_bit(false); // start
    for (int b = 0; b < 8; ++b) write_bit(((wire[i] >> b) & 1u) != 0);
    write_bit(true); // stop
  }
  rflink_ramses_ii::Frame check;
  assert(rflink_ramses_ii::decodePacked(packed.data(), packed.size(), check));
  assert(check.command == command);
  return packed;
}
} // namespace synthetic_ramses
