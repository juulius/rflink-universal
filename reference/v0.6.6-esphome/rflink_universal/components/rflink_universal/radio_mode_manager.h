#pragma once
// RFLink Universal 868-MHz radio lease coordinator (RAMSES3 foundation).
// Host-testable, non-blocking, no direct SPI/UART/GPIO work in this class.
// SAFE DEFAULT: unarmed with no hardware backend. Legacy Itho stays untouched.
// A future hardware backend must implement every step and MUST verify Itho
// restoration (including IRQ/GDO mapping and ESPHome CC1101 register state).
#include <cstdint>

namespace esphome { namespace rflink_universal {

class RadioModeManager {
 public:
  enum class State : uint8_t { ITHO, ACQUIRING, RAMSES_RX, RESTORING, FAULT };
  enum class Error : uint8_t { NONE, DISABLED, BUSY, INVALID_DURATION, PAUSE_FAILED,
                                RAMSES_START_FAILED, RAMSES_STOP_FAILED,
                                ITHO_RESTORE_FAILED, ITHO_VERIFY_FAILED };
  class Backend {
   public:
    virtual ~Backend() = default;
    // These methods must be fast, bounded, serialize all 868-MHz SPI access,
    // quiesce/restart packet IRQ+RX, and NEVER call a competing CC1101 driver.
    virtual bool pause_itho() = 0;
    virtual bool start_ramses_rx() = 0;
    virtual bool stop_ramses() = 0;
    virtual bool restore_itho() = 0;
    virtual bool verify_itho() = 0;
  };
  static constexpr uint32_t MAX_LEASE_MS = 30000U;
  static constexpr uint32_t MIN_LEASE_MS = 20U;

  // Bind/arm only after a physical adapter passed hardware validation.
  // Not exposed by the current firmware/web interface; no TX commands here.
  bool arm(Backend *backend) {
    if (state_ != State::ITHO || !backend || backend_) return false;
    backend_ = backend;
    return true;
  }
  bool disarm() {
    if (state_ != State::ITHO) return false;
    backend_ = nullptr;
    return true;
  }
  bool request_ramses_rx(uint32_t now_ms, uint32_t duration_ms) {
    if (!backend_) { error_ = Error::DISABLED; return false; }
    if (state_ != State::ITHO) { error_ = Error::BUSY; return false; }
    if (duration_ms < MIN_LEASE_MS || duration_ms > MAX_LEASE_MS) {
      error_ = Error::INVALID_DURATION;
      return false;
    }
    start_ms_ = now_ms;
    lease_ms_ = duration_ms;
    state_ = State::ACQUIRING; // reserve ownership before returning
    error_ = Error::NONE;
    return true;
  }
  void request_restore() {
    if (state_ == State::RAMSES_RX || state_ == State::ACQUIRING)
      state_ = State::RESTORING;
  }
  // Called from the device's single event loop. No waiting or blocking retries.
  void poll(uint32_t now_ms) {
    if (!backend_) return;
    if (state_ == State::ACQUIRING) {
      if (!backend_->pause_itho()) { error_ = Error::PAUSE_FAILED; state_ = State::RESTORING; }
      else if (!backend_->start_ramses_rx()) { error_ = Error::RAMSES_START_FAILED; state_ = State::RESTORING; }
      else state_ = State::RAMSES_RX;
    }
    if (state_ == State::RAMSES_RX && static_cast<uint32_t>(now_ms - start_ms_) >= lease_ms_)
      state_ = State::RESTORING;
    if (state_ == State::RESTORING) {
      // Never report ITHO until the whole original register/IRQ/UART state
      // has been restored and verified. No packet RX/TX in any other state.
      const bool stopped = backend_->stop_ramses();
      const bool restored = backend_->restore_itho();
      const bool verified = restored && backend_->verify_itho();
      if (!restored) error_ = Error::ITHO_RESTORE_FAILED;
      else if (!verified) error_ = Error::ITHO_VERIFY_FAILED;
      else if (!stopped && error_ == Error::NONE) error_ = Error::RAMSES_STOP_FAILED;
      if (stopped && restored && verified) {
        state_ = State::ITHO;
        ++successful_restores_;
      } else {
        state_ = State::FAULT; // sticky fault until explicit recovery
        ++failed_restores_;
      }
    }
  }
  // Explicit bounded recovery request from a future hardware controller.
  // A failed restore remains a hard RF block, never quietly clears itself.
  bool try_recover() {
    if (state_ != State::FAULT || !backend_) return false;
    state_ = State::RESTORING;
    error_ = Error::NONE;
    return true;
  }
  bool itho_ready() const { return state_ == State::ITHO; }
  bool ramses_listening() const { return state_ == State::RAMSES_RX; }
  bool busy() const { return state_ != State::ITHO; }
  State state() const { return state_; }
  Error last_error() const { return error_; }
  uint32_t successful_restores() const { return successful_restores_; }
  uint32_t failed_restores() const { return failed_restores_; }
  bool hardware_adapter_present() const { return backend_ != nullptr; }
 private:
  Backend *backend_{nullptr};
  State state_{State::ITHO};
  Error error_{Error::NONE};
  uint32_t start_ms_{0}, lease_ms_{0};
  uint32_t successful_restores_{0}, failed_restores_{0};
};

}} // namespace esphome::rflink_universal
