#pragma once
// Protocol-neutral TX plans. Encoder code lives in its decoder pack; the core
// alone owns MQTT, user opt-in, radio readiness and actual radio dispatch.
#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include "device_store.h"
#include "radio_contract_v2.h"
#include "esphome/components/remote_base/remote_base.h"

namespace esphome { namespace rflink_universal { namespace tx_engine {
struct Plan {
  FrameKind kind{FrameKind::RAW};
  TxTransport transport{TxTransport::NONE}; // preference; actual route selected at dispatch
  std::string protocol; // bound to approved manifest; not supplied by the web client
  uint8_t radio{255};
  uint8_t repeats{0};
  uint16_t repeat_gap_ms{0};  // Packet protocols may need a quiet gap between repeats.
  uint32_t frequency_hz{0}; // approved manifest frequency, checked against physical channel
  uint64_t identity{0};
  std::array<uint8_t, 4> args{{0,0,0,0}};
  // Protocol state that must be committed before the RF command is queued.
  // Rolling-code encoders use this to guarantee that a code is never reused
  // after a reboot, even if the actual over-the-air transmission fails.
  std::vector<FieldValue> state_updates;
};
class Encoder {
 public:
  virtual ~Encoder() = default;
  virtual const char *protocol() const = 0;
  virtual const char *encoder_id() const = 0;
  // Plan a user-approved action against an immutable TX snapshot when enabled.
  virtual bool prepare(const DeviceRecord &, const std::string &, Plan &) const = 0;
  // Produce a snapshot BEFORE changing the stored TX opt-in flag.
  virtual bool capture(const DeviceRecord &, std::vector<FieldValue> &) const = 0;
  // Encoders explicitly implement the transport they use. Returning false on
  // the wrong transport prevents a pack from silently sending an empty frame.
  virtual bool encode_raw(remote_base::RemoteTransmitData *, const Plan &) const { return false; }
  virtual bool encode_packet(std::vector<uint8_t> &, const Plan &) const { return false; }
  virtual bool encode_serial(std::vector<uint8_t> &, const Plan &) const { return false; }
};
void register_once(Encoder *encoder);
void ensure_linked();
const Encoder *resolve(const std::string &protocol);
bool prepare(const DeviceRecord &, const std::string &, Plan &, const Encoder *&);
bool eligible(const DeviceRecord &);
class Registrar { public: explicit Registrar(Encoder *p) { register_once(p); } };
#define RFLINK_REGISTER_TX_ENCODER(CLASS_NAME) \
  static CLASS_NAME g_##CLASS_NAME##_instance; \
  static ::esphome::rflink_universal::tx_engine::Registrar g_##CLASS_NAME##_registrar(&g_##CLASS_NAME##_instance); \
  extern "C" ::esphome::rflink_universal::tx_engine::Encoder *rflink_tx_anchor_##CLASS_NAME() { return &g_##CLASS_NAME##_instance; }
}}} // namespace esphome::rflink_universal::tx_engine
