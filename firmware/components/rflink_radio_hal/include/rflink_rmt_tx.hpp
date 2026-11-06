#pragma once
#include "rflink_radio_manager.hpp"
#include "rflink_tx_session.hpp"
namespace rflink {
class Cc1101GdoRmtTx {
public:
  explicit Cc1101GdoRmtTx(Cc1101SpiBus& bus):bus_(bus){}
  PhysicalTxResult transmit(RadioId radio, const RfProfile& p, const EncodedTxFrame& frame);
private:
  Cc1101SpiBus& bus_;
};
} // namespace rflink
