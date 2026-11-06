#pragma once
#include <array>
#include "rflink_cc1101_spi.hpp"
#include "rflink_rf_profiles.hpp"
namespace rflink {
// Shared lifetime guard for non-UART TX transports. The owner (RadioManager)
// has already stopped any RX; this object snapshots touched modem registers
// and the full PA table, and restores everything even on error.
class Cc1101TxSession {
public:
  Cc1101TxSession(Cc1101SpiBus& bus, RadioId radio);
  bool prepare(const RfProfile& profile, bool asynchronous, uint8_t length);
  bool restore();
  bool touched() const {return touched_;}
private:
  bool set_checked(uint8_t address,uint8_t value);
  bool snapshot(uint8_t address);
  Cc1101SpiBus& bus_;
  RadioId radio_;
  std::array<uint8_t,47> old_{};
  std::array<bool,47> saved_{};
  std::array<uint8_t,8> old_pa_{};
  bool pa_saved_ = false;
  bool touched_ = false;
};
} // namespace rflink
