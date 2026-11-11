#pragma once
// RAMSES-II / Orcon 15RF protocol TX preparation. Pure C++11, no RF I/O.
// DELIBERATELY NOT a registered RFLink TX encoder: the board currently exposes
// only 63-byte Itho packet TX, not RAMSES asynchronous UART/GDO0 transport.
#include "codec.h"
#include <stddef.h>
#include <stdint.h>
#include <vector>

namespace rflink_ramses_ii {
namespace tx {
struct Ids {
  uint32_t controller; // distinct, persistent virtual remote, class 37
  uint32_t fan;        // paired fan's 24-bit RAMSES ID (not an RX remote ID)
};

enum class Action { AWAY, LOW, MEDIUM, HIGH, AUTO, TIMER_15, TIMER_30, TIMER_60 };

// MVS-15R layout derived from documented ramses_cc Orcon remote telegrams.
// Fan and controller address must differ. A bound fan ID must never be guessed.
bool buildCommand(const Ids& ids, Action action, Frame& output);
// 1FC9 three-offer handshake: offer, fan ACK, controller confirmation, 10E0 info.
bool buildBindOffer(uint32_t virtual_remote_id, Frame& output);
bool buildBindConfirm(uint32_t virtual_remote_id, uint32_t fan_id, Frame& output);
bool buildDeviceInfo(uint32_t virtual_remote_id, Frame& output);
// Assert the binding ACK targets THIS virtual remote; never accept unrelated RF.
bool isBindAcknowledgement(const Frame& received, uint32_t virtual_remote_id, uint32_t& fan_id);

// Transport-neutral frame builders for later asynchronous CC1101 UART radio
// driver. The radio must transmit a continuous UART 8N1 data stream at 38383
// baud, not the existing Itho 63-byte CC1101 FIFO packet transport.
bool encodeLogical(const Frame& frame, std::vector<uint8_t>& logical);
bool encodeWire(const Frame& frame, std::vector<uint8_t>& wire);
// Exact UART waveform packed MSB-first, optional FF preamble UART bytes.
// bitCount is the meaningful number of bits; no CC1101 radio writes happen.
bool encodeOnAir(const Frame& frame, unsigned preamble_bytes,
                 std::vector<uint8_t>& packed, size_t& bitCount);

class PairingSession {
 public:
  enum class State { IDLE, OFFER_READY, WAITING_ACK, CONFIRM_READY,
                     WAITING_VERIFICATION, VERIFIED, EXPIRED };
  PairingSession();
  // Caller must persist a random nonzero class-37 virtual remote beforehand.
  bool begin(uint32_t virtual_remote_id, uint64_t now_ms);
  bool makeOffer(Frame& out) const;
  void offerSent(uint64_t now_ms);
  bool receive(const Frame& frame, uint64_t now_ms);
  bool makeConfirmation(Frame& out) const;
  void confirmationSent(uint64_t now_ms);
  // Only call after verified physical device response / operator confirmation.
  bool verify(uint32_t confirmed_fan_id, uint64_t now_ms);
  void checkTimeout(uint64_t now_ms);
  State state() const { return state_; }
  uint32_t controller() const { return controller_; }
  uint32_t fan() const { return fan_; }
 private:
  State state_;
  uint32_t controller_, fan_;
  uint64_t last_ms_;
};
}  // namespace tx
}  // namespace rflink_ramses_ii
