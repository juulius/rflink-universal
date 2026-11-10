#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include "esp_err.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "rflink_radio_hal.hpp"

namespace rflink {
// This layer owns SPI and CS, but deliberately does NOT install a GDO ISR,
// restore modem profiles, or promise packet reception.
struct SpiHealthCounters {
  uint32_t ready_timeouts = 0;
  uint32_t transfer_errors = 0;
  // Raw last completed identity sample, or 0xFF when the read failed.
  uint8_t last_partnum = 0xff;
  uint8_t last_version = 0xff;
  // A read/write/restore CHANNR challenge is performed only at cold boot.
  bool boot_readback_verified = false;
};

struct ProbeResult {
  esp_err_t error = ESP_ERR_INVALID_STATE;
  uint8_t partnum = 0xff;
  uint8_t version = 0xff;
  bool plausible = false;
  uint8_t marcstate = 0xff;
  uint8_t rxbytes = 0xff;
  bool reset_issued = false;
  bool readback_checked = false;
  bool readback_verified = false;
  uint8_t readback_observed = 0xff;
};

class Cc1101SpiBus final {
 public:
  Cc1101SpiBus() = default;
  ~Cc1101SpiBus();
  Cc1101SpiBus(const Cc1101SpiBus&) = delete;
  Cc1101SpiBus& operator=(const Cc1101SpiBus&) = delete;
  esp_err_t begin();
  ProbeResult probe(RadioId id);
  // At boot ONLY: CC1101 SRES followed by PARTNUM/VERSION check.
  // Does not set an RF profile or enable RX; never call during TX/RX.
  ProbeResult boot_reset_and_probe(RadioId id);
  // Non-invasive identity read: never resets the modem or mutates the runtime mode.
  esp_err_t read_identity(RadioId id,uint8_t* partnum,uint8_t* version);
  esp_err_t read_runtime_status(RadioId id, uint8_t* marcstate, uint8_t* rxbytes);
  // Caller MUST own this radio's manager lease and stop its prior RX/TX
  // transport first. Bounded IDLE -> RX transition and PLL verification,
  // following the validated legacy CC1101 receiver handover. Never SRES here.
  esp_err_t enter_rx_verified(RadioId id);
  // CC1101 RX FIFO burst (0x3F), limit 63 bytes per transfer.
  esp_err_t read_rx_fifo(RadioId id, uint8_t* out, size_t length);
  // 64-byte hardware FIFO, bounded atomic burst (63 bytes + header).
  esp_err_t write_tx_fifo(RadioId id, const uint8_t* src, size_t length);
  esp_err_t read_tx_fifo_status(RadioId id, uint8_t* txbytes);
  esp_err_t read_pa_table(RadioId id, uint8_t* out, size_t length);
  esp_err_t write_pa_table(RadioId id, const uint8_t* src, size_t length);
  esp_err_t read_register(RadioId id, uint8_t addr, uint8_t* value);
  esp_err_t write_register(RadioId id, uint8_t addr, uint8_t value);
  // Strobes affect the radio state. Not used during ordinary probing.
  esp_err_t strobe(RadioId id, uint8_t cmd, uint8_t* chip_status = nullptr);
  RadioState state(RadioId id) const;
  SpiHealthCounters health(RadioId id) const;
 private:
  static constexpr size_t index(RadioId id) { return static_cast<size_t>(id); }
  esp_err_t transaction(RadioId id, const uint8_t* tx, uint8_t* rx, size_t bytes);
  esp_err_t transaction_burst(RadioId id, const uint8_t* tx, uint8_t* rx, size_t bytes);
  esp_err_t read_status_register(RadioId id, uint8_t addr, uint8_t* value);
  std::array<spi_device_handle_t,3> dev_{{nullptr,nullptr,nullptr}};
  SemaphoreHandle_t lock_ = nullptr;
  bool ready_ = false;
  std::array<RadioState,3> states_{};
  std::array<std::atomic<uint32_t>,3> ready_timeouts_{};
  std::array<std::atomic<uint32_t>,3> transfer_errors_{};
  // Atomic packed sample: bit16 valid, bits15-8 PARTNUM, bits7-0 VERSION.
  std::array<std::atomic<uint32_t>,3> last_identity_{};
  std::array<std::atomic<bool>,3> boot_readback_ok_{};
};
}  // namespace rflink
