#pragma once
#include <cstdint>
#include <vector>
namespace esphome { namespace cc1101 {
enum class CC1101Error : uint8_t { NONE, FAILED };
class CC1101Component {
 public:
  void enable() {}
  void disable() {}
  void write_byte(uint8_t value) { last_spi_write = value; }
  uint8_t read_byte() { return spi_read_value; }
  uint8_t last_spi_write{0};
  uint8_t spi_read_value{0x14};
  CC1101Error transmit_packet(const std::vector<uint8_t> &packet) {
    ++sent_count;
    last_packet = packet;
    return transmit_result;
  }
  unsigned sent_count{0};
  std::vector<uint8_t> last_packet;
  CC1101Error transmit_result{CC1101Error::NONE};
};
} }
