#include "rflink_itho_codec.hpp"

#include <algorithm>

namespace rflink {
namespace itho {

static constexpr std::array<uint8_t, 5> HEADER{{0x00, 0xB3, 0x2A, 0xAB, 0x2A}};

uint8_t checksum_negative_sum(const std::vector<uint8_t> &bytes) {
  uint8_t sum = 0;
  for (auto value : bytes) sum = static_cast<uint8_t>(sum + value);
  return static_cast<uint8_t>(0u - sum);
}

static std::vector<uint8_t> decode_manchesterish_(const std::vector<uint8_t> &packet) {
  std::vector<uint8_t> decoded(32, 0);
  size_t out_i = 0;
  uint8_t out_j = 4;
  uint8_t in_bitcounter = 0;
  for (size_t in_i = HEADER.size(); in_i < packet.size() && out_i < decoded.size(); in_i++) {
    for (int bit = 7; bit >= 0 && out_i < decoded.size(); bit--) {
      const bool use = in_bitcounter == 0 || in_bitcounter == 2 || in_bitcounter == 4 || in_bitcounter == 6;
      if (use) {
        const uint8_t value = static_cast<uint8_t>((packet[in_i] >> bit) & 1u);
        decoded[out_i] |= static_cast<uint8_t>(value << out_j);
        out_j++;
        if (out_j > 7) out_j = 0;
        if (out_j == 4) out_i++;
      }
      in_bitcounter++;
      if (in_bitcounter > 9) in_bitcounter = 0;
    }
  }
  decoded.resize(out_i);
  return decoded;
}

bool decode_packet(const std::vector<uint8_t> &packet, Decoded &out) {
  out = Decoded{};
  if (packet.size() != 63 || !std::equal(HEADER.begin(), HEADER.end(), packet.begin())) return false;
  const auto d = decode_manchesterish_(packet);
  if (d.size() < 16) return false;

  out.source_id = {{d[1], d[2], d[3]}};
  out.counter = d[4];

  if (d.size() > 10 && d[5] == 0x22 && d[7] == 0x03) {
    if (d[6] == 0xF1 && d[10] == 0x04) {
      out.type = MessageType::SPEED;
      out.speed = d[9];
      out.valid = out.speed >= 1 && out.speed <= 4;
      return out.valid;
    }
    if (d[6] == 0xF3 && d[9] == 0x00 && (d[10] == 0x0A || d[10] == 0x14 || d[10] == 0x1E)) {
      out.type = MessageType::TIMER;
      out.timer_minutes = d[10];
      out.valid = true;
      return true;
    }
  }
  if (d.size() > 13 && d[0] == 0x1A && d[12] == 0x06) {
    out.type = MessageType::UNIT_SPEED;
    out.fan_percent = static_cast<uint8_t>(d[13] / 2u);
    out.valid = true;
    return true;
  }
  if (d.size() > 15 && d[0] == 0x18 && d[12] == 0x00) {
    out.type = MessageType::HUMIDITY;
    out.humidity_percent = d[15];
    out.valid = true;
    return true;
  }
  return false;
}

std::vector<uint8_t> encode_payload(const std::vector<uint8_t> &payload, uint8_t end_byte) {
  std::vector<uint8_t> out(63, 0xAA);
  std::copy(HEADER.begin(), HEADER.end(), out.begin());
  size_t out_byte = HEADER.size();
  uint8_t out_bit = 0;
  uint8_t pattern = 0;
  uint8_t bit_select = 4;

  auto write_bit = [&](bool bit) {
    if (out_byte >= out.size()) return;
    const uint8_t shift = static_cast<uint8_t>(7u - out_bit);
    if (bit) out[out_byte] |= static_cast<uint8_t>(1u << shift);
    else out[out_byte] &= static_cast<uint8_t>(~(1u << shift));
    out_bit++;
    if (out_bit == 8) { out_bit = 0; out_byte++; }
  };

  for (size_t source_byte = 0; source_byte < payload.size(); ) {
    if (pattern == 8) {
      write_bit(true);
      write_bit(false);
      pattern = 0;
      continue;
    }
    const bool bit = ((payload[source_byte] >> bit_select) & 1u) != 0;
    write_bit(bit);
    write_bit(!bit);
    pattern = static_cast<uint8_t>(pattern + 2);
    bit_select++;
    if (bit_select > 7) bit_select = 0;
    if (bit_select == 4) source_byte++;
  }

  while (out_bit != 0 && out_byte < out.size()) {
    write_bit(true);
    if (out_bit != 0) write_bit(false);
  }
  if (out_byte < out.size()) out[out_byte++] = end_byte;
  while (out_byte < out.size()) out[out_byte++] = 0xAA;
  return out;
}

static void append_id_(std::vector<uint8_t> &p, const std::array<uint8_t, 3> &id) {
  p.insert(p.end(), id.begin(), id.end());
}

std::vector<uint8_t> build_speed_payload(uint8_t device_type, const std::array<uint8_t, 3> &id,
                                         uint8_t counter, SpeedCommand command) {
  std::vector<uint8_t> p;
  p.push_back(device_type); append_id_(p, id); p.push_back(counter);
  p.insert(p.end(), {0x22, 0xF1, 0x03, 0x00, static_cast<uint8_t>(command), 0x04});
  p.push_back(checksum_negative_sum(p));
  return p;
}

std::vector<uint8_t> build_timer_payload(uint8_t device_type, const std::array<uint8_t, 3> &id,
                                         uint8_t counter, uint8_t minutes) {
  if (minutes != 10 && minutes != 20 && minutes != 30) minutes = 10;
  std::vector<uint8_t> p;
  p.push_back(device_type); append_id_(p, id); p.push_back(counter);
  p.insert(p.end(), {0x22, 0xF3, 0x03, 0x00, 0x00, minutes});
  p.push_back(checksum_negative_sum(p));
  return p;
}

std::vector<uint8_t> build_join_payload(uint8_t device_type, const std::array<uint8_t, 3> &id,
                                        uint8_t counter, bool compatibility) {
  std::vector<uint8_t> p;
  p.push_back(device_type); append_id_(p, id); p.push_back(counter);
  p.push_back(0x1F); p.push_back(0xC9); p.push_back(0x0C);
  if (compatibility) p.insert(p.end(), {0x63, 0x22, 0xF8});
  else p.insert(p.end(), {0x00, 0x22, 0xF1});
  append_id_(p, id);
  p.insert(p.end(), {0x01, 0x10, 0xE0});
  append_id_(p, id);
  p.push_back(checksum_negative_sum(p));
  return p;
}

std::vector<uint8_t> build_leave_payload(uint8_t device_type, const std::array<uint8_t, 3> &id,
                                         uint8_t counter) {
  std::vector<uint8_t> p;
  p.push_back(device_type); append_id_(p, id); p.push_back(counter);
  p.insert(p.end(), {0x1F, 0xC9, 0x06, 0x00, 0x1F, 0xC9});
  append_id_(p, id);
  p.push_back(checksum_negative_sum(p));
  return p;
}

}  // namespace itho
}  // namespace rflink
