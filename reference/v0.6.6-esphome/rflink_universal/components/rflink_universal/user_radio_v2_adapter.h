#pragma once
// Runtime compatibility view: legacy NVS UserDecoder records already declare
// a radio and an input kind. No storage migration or elevated TX permission.
#include "user_library.h"
#include "radio_contract_v2.h"

namespace esphome { namespace rflink_universal {
inline bool user_decoder_accepts_v2(const UserDecoder &user, const RFFrame &frame) {
  return user.enabled && user.radio == frame.channel && user.kind == frame.kind;
}
// Map existing persisted user recipes to the SAME transport selector as the
// built-in encoders. The migration adds no permissions and changes no NVS bytes.
inline RadioTxSpec legacy_user_tx_spec(const UserDecoder &user) {
  return {user.id.c_str(), user.radio,
          user.kind == FrameKind::RAW ? TxTransport::RMT_RAW : TxTransport::SPI_FIFO_PACKET,
          TxTransport::NONE,
          user.kind == FrameKind::RAW ? "legacy_user_ook" : "legacy_user_packet",
          user.enabled && !user.tx_source.empty()};
}
inline const char *legacy_user_capture_method(const UserDecoder &user) {
  return user.kind==FrameKind::RAW ? "raw_rmt" : "spi_packet";
}
}} // namespace
