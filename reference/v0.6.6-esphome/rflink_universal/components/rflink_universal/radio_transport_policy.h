#pragma once
// Generic, decoder-declared radio transport selection. This is not an Orcon driver.
// The receiver engine chooses a declared hardware transport; the radio lease
// coordinator ensures only one mode owns a physical CC1101 at any time.
#include <cstdint>
#include <cstddef>
#include <vector>

namespace esphome { namespace rflink_universal {

enum class RxTransport : uint8_t {
  NONE = 0, SPI_PACKET, SPI_FIFO_SYNC, SPI_FIFO_RAW, UART_STREAM
};

inline const char *transport_name(RxTransport mode) {
  switch (mode) {
    case RxTransport::SPI_PACKET: return "spi_packet";
    case RxTransport::SPI_FIFO_SYNC: return "spi_fifo_sync";
    case RxTransport::SPI_FIFO_RAW: return "spi_fifo_raw";
    case RxTransport::UART_STREAM: return "uart_stream";
    default: return "none";
  }
}

struct DecoderRadioSpec {
  const char *protocol;
  uint8_t radio_index;      // 0=433.42, 1=433.92, 2=868.30
  RxTransport preferred;
  RxTransport fallback;     // NONE means no fallback
  uint16_t max_window_ms;   // upper limit is also enforced by RadioModeManager
  uint32_t scan_every_ms;    // 0 = no automatic scanning (safe default)
  uint16_t scan_window_ms;   // receive-only lease length
};

class RadioTransportPolicy {
 public:
  enum class Decision : uint8_t {
    SELECTED, NO_DECODER, RADIO_BUSY, UNAVAILABLE, DISABLED, INVALID_WINDOW
  };
  struct Result {
    Decision decision;
    RxTransport transport;
    uint8_t radio_index;
  };
  explicit RadioTransportPolicy(const DecoderRadioSpec *specs, size_t count)
    : specs_(specs), count_(count) {}

  const DecoderRadioSpec *find(const char *protocol) const {
    for(size_t i=0;i<count_;++i)
      if(same(specs_[i].protocol,protocol))return specs_+i;
    return nullptr;
  }
  Result choose(const char *protocol, uint8_t radio_index,
                uint32_t supported_mask, bool radio_busy, bool enabled,
                uint32_t window_ms) const {
    if (!protocol) return {Decision::NO_DECODER,RxTransport::NONE,radio_index};
    const DecoderRadioSpec *spec=nullptr;
    for(size_t i=0;i<count_;++i) {
      if (specs_[i].radio_index==radio_index && same(specs_[i].protocol,protocol)) {
        spec=&specs_[i];break;
      }
    }
    if (!spec) return {Decision::NO_DECODER,RxTransport::NONE,radio_index};
    if (!enabled) return {Decision::DISABLED,RxTransport::NONE,radio_index};
    if (radio_busy) return {Decision::RADIO_BUSY,RxTransport::NONE,radio_index};
    if (window_ms<20 || window_ms>spec->max_window_ms || window_ms>30000)
      return {Decision::INVALID_WINDOW,RxTransport::NONE,radio_index};
    if (supports(supported_mask,spec->preferred))
      return {Decision::SELECTED,spec->preferred,radio_index};
    if (supports(supported_mask,spec->fallback))
      return {Decision::SELECTED,spec->fallback,radio_index};
    return {Decision::UNAVAILABLE,RxTransport::NONE,radio_index};
  }
  const DecoderRadioSpec *at(size_t i) const {return i<count_?specs_+i:nullptr;}
  size_t count() const {return count_;}
  static constexpr uint32_t bit(RxTransport t) {
    return t==RxTransport::NONE ? 0u : (1u << static_cast<unsigned>(t));
  }
 private:
  static bool supports(uint32_t mask,RxTransport mode) {
    return (mask & bit(mode))!=0;
  }
  static bool same(const char *a,const char *b) {
    if (!a || !b) return false;
    while (*a && *b) { if (*a++!=*b++) return false; }
    return *a==*b;
  }
  const DecoderRadioSpec *specs_{nullptr};
  size_t count_{0};
};
// Deliberately agnostic to SPI/UART and protocol names. Decoder manifests
// declare time slices; the owner must call mark() only after accepting a lease.
class RadioScanScheduler {
 public:
  const DecoderRadioSpec *due(const RadioTransportPolicy &policy,uint32_t now_ms,
                              bool radio_idle,bool enabled) {
    if(!enabled || !radio_idle)return nullptr;
    prepare(policy.count());
    for(size_t i=0;i<policy.count();++i) {
      const auto *p=policy.at(i);
      if(p && p->scan_every_ms && p->scan_window_ms &&
         (!started_[i] || uint32_t(now_ms-last_ms_[i])>=p->scan_every_ms))
        return p;
    }
    return nullptr;
  }
  void mark(const RadioTransportPolicy &policy,const char *protocol,uint32_t now_ms) {
    prepare(policy.count());
    for(size_t i=0;i<policy.count();++i) {
      const auto *p=policy.at(i);
      if(p && match(p->protocol,protocol)) {started_[i]=true;last_ms_[i]=now_ms;return;}
    }
  }
 private:
  void prepare(size_t count) {
    // Preserve timestamps when more decoders are registered. Do not silently
    // ignore index 16+ as the original fixed-size scheduler did.
    if(last_ms_.size()<count) {last_ms_.resize(count,0u);started_.resize(count,0u);}
  }
  static bool match(const char *a,const char *b) {
    if(!a || !b)return false;
    while(*a && *b){if(*a++!=*b++)return false;}
    return *a==*b;
  }
  std::vector<uint32_t> last_ms_;
  std::vector<uint8_t> started_;
};
}}  // namespace
