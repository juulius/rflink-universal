#pragma once
#include <cstddef>
#include <cstdint>
#include "rflink_radio_hal.hpp"

namespace rflink {
// Hardware RF requirements are separate from the decoder/protocol itself.
enum class RxTransport : uint8_t { packet_fifo, gdo_rmt, gdo_serial };
enum class TxTransport : uint8_t { packet_fifo, gdo_rmt, gdo_uart };
enum class RfProfileId : uint8_t { itho_868_packet, ramses_868_packet, ook_43342_pulses, ook_43392_pulses, packet_868_bench };
struct RegisterSetting { uint8_t address; uint8_t value; };
// UART is only a physical serializer; wire encoding belongs to the decoder.
struct UartTxPreset {
  uint32_t baud;
  uint8_t pktctrl0, mdmcfg4, mdmcfg3, mdmcfg2, deviatn;
};
struct RfProfile {
  RfProfileId id;
  const char* name;
  uint32_t frequency_hz;
  Modulation modulation;
  RxTransport rx_transport;
  TxTransport tx_transport;
  const RegisterSetting* modem_registers;
  size_t register_count;
  uint8_t packet_length;
  const UartTxPreset* uart_tx;
  // PA-table level is protocol/profile-specific and must be bench validated.
  // nullptr means TX is explicitly not available for RMT/FIFO profiles.
  const uint8_t* pa_table;
  size_t pa_table_len;
  // Profiles requiring GDO/RMT must be installed by the corresponding
  // hardware transport, never by the FIFO RX path.
};
const RfProfile& rf_profile(RfProfileId id);
bool profile_compatible_with_radio(const RfProfile& p, RadioId radio);
// Reject placeholders: only completely configured, supported hardware profiles may be activated.
bool profile_has_registers(const RfProfile& p);
bool uart_tx_preset_valid(const RfProfile& p);
bool rmt_tx_preset_valid(const RfProfile& p);
bool packet_tx_preset_valid(const RfProfile& p);
// Convert carrier frequency to CC1101 FREQ[23:0], 26 MHz reference.
uint32_t cc1101_frequency_word(uint32_t frequency_hz);
}
