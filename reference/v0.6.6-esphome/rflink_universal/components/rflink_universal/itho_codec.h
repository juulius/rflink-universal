#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace esphome {
namespace rflink_universal {
namespace itho {

enum class SpeedCommand : uint8_t { STANDBY = 1, LOW = 2, MEDIUM = 3, HIGH = 4 };

enum class MessageType : uint8_t { UNKNOWN = 0, SPEED, TIMER, UNIT_SPEED, HUMIDITY };

struct Decoded {
  bool valid{false};
  MessageType type{MessageType::UNKNOWN};
  std::array<uint8_t, 3> source_id{{0, 0, 0}};
  uint8_t counter{0};
  uint8_t speed{0};
  uint8_t timer_minutes{0};
  uint8_t fan_percent{0};
  uint8_t humidity_percent{0};
};

bool decode_packet(const std::vector<uint8_t> &packet, Decoded &out);
std::vector<uint8_t> encode_payload(const std::vector<uint8_t> &payload, uint8_t end_byte);
std::vector<uint8_t> build_speed_payload(uint8_t device_type, const std::array<uint8_t, 3> &id,
                                         uint8_t counter, SpeedCommand command);
std::vector<uint8_t> build_timer_payload(uint8_t device_type, const std::array<uint8_t, 3> &id,
                                         uint8_t counter, uint8_t minutes);
std::vector<uint8_t> build_join_payload(uint8_t device_type, const std::array<uint8_t, 3> &id,
                                        uint8_t counter, bool compatibility);
std::vector<uint8_t> build_leave_payload(uint8_t device_type, const std::array<uint8_t, 3> &id,
                                         uint8_t counter);
uint8_t checksum_negative_sum(const std::vector<uint8_t> &bytes);

}  // namespace itho
}  // namespace rflink_universal
}  // namespace esphome
