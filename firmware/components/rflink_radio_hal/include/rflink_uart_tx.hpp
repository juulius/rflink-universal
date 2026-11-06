#pragma once
#include "rflink_cc1101_spi.hpp"
#include "rflink_radio_manager.hpp"
namespace rflink {
// Protocol-agnostic physical output. The decoder must supply encoded wire
// bytes and a reviewed RF profile. It never generates a RAMSES/Itho command.
class Cc1101GdoUartTx final {
public:
  explicit Cc1101GdoUartTx(Cc1101SpiBus& bus) : bus_(bus) {}
  PhysicalTxResult transmit(RadioId radio, const RfProfile& profile, const EncodedTxFrame& frame);
private:
  Cc1101SpiBus& bus_;
};
} // namespace rflink
