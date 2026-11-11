#pragma once
// Protocol-independent serial TX hardware contract for CC1101 GDO0 (or a
// future radio with the same waveform interface). Encoder supplies wire bytes;
// adapter owns GPIO/UART, radio lease, TX/RX transitions and verification.
// No physical adapter is registered by default; this alone sends nothing.
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
namespace esphome { namespace rflink_universal {
class SerialTxAdapter {
 public:
  virtual ~SerialTxAdapter() = default;
  // True only if a physical adapter was initialized and measured to support
  // the named, reviewed preset. Fail closed after sticky hardware errors.
  virtual bool ready(uint8_t radio, const char *approved_profile) const = 0;
  // send MUST be synchronous: acquire exclusive radio ownership, prepare TX,
  // output complete wire data, restore RX, verify RX. Return false on failure.
  // Error messages must distinguish TX failure from restoration failure.
  virtual bool send(uint8_t radio, const char *approved_profile,
                    const uint8_t *wire, size_t count,
                    uint8_t repeats, uint16_t gap_ms,
                    std::string &error) = 0;
};
inline bool serial_wire_sane(const std::vector<uint8_t> &wire,
                             uint8_t repeats, uint16_t gap_ms) {
  return wire.size() >= 4 && wire.size() <= 512 &&
         repeats >= 1 && repeats <= 10 && gap_ms <= 1000;
}
}} // namespaces
