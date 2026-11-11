#pragma once
// Radio Engine v2: immutable decoder-input metadata, shared by all three radios.
// This does not configure radio hardware and never authorizes transmission.
#include "decoder.h"
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace esphome { namespace rflink_universal {
enum class InputKind : uint8_t { RAW, PACKET };
// Protocol-neutral transmit transport selection. No protocol name or model is
// special-cased by the physical radio dispatcher.
enum class TxTransport : uint8_t { NONE, RMT_RAW, SPI_FIFO_PACKET, GDO_SERIAL };
struct RadioTxSpec {
  const char *protocol;
  uint8_t channel;
  TxTransport preferred;
  TxTransport fallback;
  const char *rf_profile; // Reviewed physical preset; not arbitrary register writes.
  bool enabled;           // Mirrors capabilities; cannot grant transmit authority.
};
inline bool tx_transport_kind_compatible(TxTransport route, FrameKind kind) {
  return route == TxTransport::RMT_RAW ? kind == FrameKind::RAW :
         (route == TxTransport::SPI_FIFO_PACKET || route == TxTransport::GDO_SERIAL) ?
           kind == FrameKind::PACKET : false;
}
// Eligibility is independent of protocol and independent of UI state. Only an
// available, reviewed hardware transport may be selected.
inline TxTransport choose_tx_transport(const RadioTxSpec &spec, FrameKind kind,
                                       bool rmt_available, bool spi_available,
                                       bool gdo_available) {
  if (!spec.enabled) return TxTransport::NONE;
  const auto available=[&](TxTransport t) {
    return tx_transport_kind_compatible(t,kind) &&
           ((t==TxTransport::RMT_RAW && rmt_available) ||
            (t==TxTransport::SPI_FIFO_PACKET && spi_available) ||
            (t==TxTransport::GDO_SERIAL && gdo_available));
  };
  if (available(spec.preferred)) return spec.preferred;
  if (available(spec.fallback)) return spec.fallback;
  return TxTransport::NONE;
}
struct RadioInputSpec {
  const char *protocol;
  uint8_t channel;
  InputKind kind;
  const char *profile;
  const char *capture_method;
};
class RadioInputIndex {
 public:
  RadioInputIndex(const RadioInputSpec *specs,size_t count) : specs_(specs),count_(count) {}
  // Fail OPEN when the decoder has no catalog entry. This preserves third-party
  // or user-defined decoder compatibility until their manifests are migrated.
  bool allows(const char *protocol,const RFFrame &frame) const {
    if(!protocol) return true;
    bool found=false;
    for(size_t i=0;i<count_;++i) {
      const auto &r=specs_[i];
      if(!r.protocol || std::strcmp(r.protocol,protocol)!=0) continue;
      found=true;
      if((r.channel==255 || r.channel==frame.channel) && ((r.kind==InputKind::RAW && frame.kind==FrameKind::RAW) ||
                                     (r.kind==InputKind::PACKET && frame.kind==FrameKind::PACKET)))
        return true;
    }
    return !found;
  }
  size_t count() const { return count_; }
 private:
  const RadioInputSpec *specs_;
  size_t count_;
};
}} // namespace
