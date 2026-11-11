#pragma once
// Hardware-independent transaction guard for a shared CC1101 UART/GDO serial TX.
// The caller owns the radio; every acquired TX session MUST restore and verify
// the original RX configuration before another radio operation is allowed.
#include <cstddef>
#include <cstdint>
#include <string>

namespace esphome { namespace rflink_universal {
class GdoSerialTxOperations {
 public:
  virtual ~GdoSerialTxOperations() = default;
  // acquire() is atomic: false means the radio was not modified.
  virtual bool acquire() = 0;
  // After acquire succeeds, all failures MUST be followed by restore + verify.
  virtual bool prepare_tx() = 0;
  virtual bool transmit_frame(const uint8_t *wire, size_t size) = 0;
  virtual void between_frames(uint16_t gap_ms) = 0;
  virtual bool restore_rx() = 0;
  virtual bool verify_rx() = 0;
  // Optional diagnostic hook; never contains frame bytes/keys or changes behavior.
  virtual void trace_stage(const char *,bool) {}
  // An adapter may return the precise hardware/ESP-IDF failure. The caller
  // copies it before restore_rx(), which may also change that state.
  virtual const char *failure_detail() const { return ""; }
};
class GdoSerialTxSession {
 public:
  explicit GdoSerialTxSession(GdoSerialTxOperations &ops) : ops_(ops) {}
  bool ready() const { return !sticky_fault_; }
  bool fault() const { return sticky_fault_; }
  // A recovery is permitted only after a separate externally verified radio
  // reset; no automatic error-clear after reboot/session failure.
  bool clear_fault_after_external_rx_verification(bool proved) {
    if (!proved) return false;
    sticky_fault_ = false;
    return true;
  }
  bool run(const uint8_t *wire, size_t size, uint8_t repeats, uint16_t gap_ms,
           std::string &error) {
    error.clear();
    if (sticky_fault_) { error="serial TX blocked: previous RX restore failed"; return false; }
    if (!wire || size < 4 || size > 512 || repeats < 1 || repeats > 10 || gap_ms > 1000) {
      error="serial TX invalid waveform or repeat limits"; return false;
    }
    const bool acquired=ops_.acquire();
    ops_.trace_stage("acquire radio and snapshot registers",acquired);
    if (!acquired) {
      error="serial TX radio is busy or UART pin unavailable";
      append_detail_(error);
      return false;
    }
    bool sent = ops_.prepare_tx();
    ops_.trace_stage("configure CC1101 modem / UART 8N1",sent);
    if (!sent) {
      error="serial TX radio preparation failed";
      append_detail_(error);
    }
    for (uint8_t i=0; sent && i<repeats; ++i) {
      if (i && gap_ms) ops_.between_frames(gap_ms);
      const bool frame_ok=ops_.transmit_frame(wire,size);
      ops_.trace_stage("UART bytes drained to radio",frame_ok);
      if (!frame_ok) {
        sent=false;
        error="serial TX frame rejected or UART timed out";
        append_detail_(error);
      }
    }
    const bool restored=ops_.restore_rx();
    ops_.trace_stage("restore previous CC1101 RX configuration",restored);
    const bool verified=restored && ops_.verify_rx();
    ops_.trace_stage("verify RX registers and radio state",verified);
    if (!verified) {
      sticky_fault_=true;
      const char *detail=ops_.failure_detail();
      error="serial TX RX restoration/verification FAILED; radio blocked until explicit verified recovery";
      if (detail && *detail) { error += ": "; error += detail; }
    }
    return sent && verified;
  }
 private:
  void append_detail_(std::string &error) const {
    const char *detail=ops_.failure_detail();
    if (detail && *detail) { error += ": "; error += detail; }
  }
  GdoSerialTxOperations &ops_;
  bool sticky_fault_{false};
};
}} // namespace
