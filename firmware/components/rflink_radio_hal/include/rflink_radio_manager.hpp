#pragma once
#include <array>
#include <cstdint>
#include <cstddef>
#include <mutex>
#include "rflink_rf_profiles.hpp"

namespace rflink {

enum class RadioOperation : uint8_t { unavailable, idle, receiving, transmitting, switching, fault };
enum class ManagerError : uint8_t { none, invalid_radio, no_hardware, busy, incompatible_profile, missing_profile, unsupported_transport, driver_failure, invalid_payload, tx_failure, restore_failure };
struct ManagedRadio {
  bool present = false;
  bool spi_healthy = false;
  RadioOperation operation = RadioOperation::unavailable;
  RfProfileId profile = RfProfileId::itho_868_packet;
  ManagerError last_error = ManagerError::none;
  uint32_t transitions = 0;
  bool online() const { return present && spi_healthy && operation != RadioOperation::unavailable && operation != RadioOperation::fault; }
};
// Decoder-facing pulse timing for GDO0/RMT TX. Each entry is an on/off pair,
// 1us resolution, 1..32767us per half-symbol. Decoder supplies timing.
struct TxPulsePair { uint16_t high_us; uint16_t low_us; };
struct EncodedTxFrame {
  const uint8_t* wire = nullptr;
  size_t length = 0;
  uint8_t repeats = 1;
  uint16_t gap_ms = 0;
  const TxPulsePair* pulses = nullptr;
  size_t pulse_count = 0;
};
enum class TxFailureStage : uint8_t { none, unsupported, snapshot, modem, gpio_uart, enter_tx, uart_write, uart_drain, fifo_write, fifo_wait, rmt_config, rmt_send, rmt_wait, restore, rx_pause };
struct PhysicalTxResult {
  bool sent = false;
  bool hardware_restored = false;
  TxFailureStage stage = TxFailureStage::unsupported;
};
// Drivers own the real ESP-IDF SPI/GDO/RMT/UART hardware. All lifecycle
// changes must go through the single manager; unsupported transports fail closed.
class RadioTransport {
public:
  virtual ~RadioTransport()=default;
  virtual bool start_packet_rx(RadioId radio, const RfProfile& profile)=0;
  virtual bool stop_packet_rx(RadioId radio)=0;
  virtual bool start_pulse_rx(RadioId radio, const RfProfile& profile)=0;
  virtual bool stop_pulse_rx(RadioId radio)=0;
  // No default fallback to packet/FIFO or pulse TX. Fail closed.
  virtual PhysicalTxResult transmit_uart(RadioId, const RfProfile&, const EncodedTxFrame&) {
    return {false, true, TxFailureStage::unsupported};
  }
  virtual PhysicalTxResult transmit_rmt(RadioId, const RfProfile&, const EncodedTxFrame&) {
    return {false, true, TxFailureStage::unsupported};
  }
  virtual PhysicalTxResult transmit_packet(RadioId, const RfProfile&, const EncodedTxFrame&) {
    return {false, true, TxFailureStage::unsupported};
  }
};
class RadioManager {
public:
  explicit RadioManager(RadioTransport& transport):transport_(transport) {}
  void set_presence(RadioId radio, bool present, bool spi_healthy);
  ManagerError start_receive(RadioId radio, RfProfileId profile);
  ManagerError stop_receive(RadioId radio);
  // Side-effect-free TX preflight: accepts a receiving radio because transmit_encoded()
  // pauses its RX when sending. No RF emitted and no radio mode change.
  ManagerError request_transmit(RadioId radio, RfProfileId profile);
  // Atomic RX -> TX -> RX transaction. Decoder provides fully serialized bytes.
  // UI/automation is NOT wired to this experimental transport yet.
  ManagerError transmit_encoded(RadioId radio, RfProfileId profile, const EncodedTxFrame& frame);
  TxFailureStage last_tx_stage(RadioId radio) const;
  ManagedRadio status(RadioId radio) const;
private:
  static constexpr unsigned index(RadioId radio){return static_cast<unsigned>(radio);}
  std::array<ManagedRadio,3> states_{};
  std::array<TxFailureStage,3> tx_stages_{};
  mutable std::mutex mutex_;
  ManagerError validate_tx(RadioId radio, RfProfileId profile, const EncodedTxFrame* frame);
  RadioTransport& transport_;
};
} // namespace rflink
