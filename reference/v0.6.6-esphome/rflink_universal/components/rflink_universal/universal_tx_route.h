#pragma once
// Central, protocol-independent TX transport resolution.
// This does not enable a transmitter: trusted catalog and per-radio hardware
// checks are BOTH required. Selection is repeated immediately before use.
#include "radio_contract_v2.h"
namespace esphome { namespace rflink_universal {
struct TxAdapterStatus {
  bool rmt{false};
  bool spi_fifo{false};
  bool gdo_serial{false};
};
inline TxTransport resolve_transmit_route(const RadioTxSpec *trusted,
                                          uint8_t radio, FrameKind frame,
                                          TxAdapterStatus available) {
  if (!trusted || !trusted->enabled || trusted->channel != radio || radio >= 3)
    return TxTransport::NONE;
  return choose_tx_transport(*trusted, frame, available.rmt,
                             available.spi_fifo, available.gdo_serial);
}
}} // namespaces
