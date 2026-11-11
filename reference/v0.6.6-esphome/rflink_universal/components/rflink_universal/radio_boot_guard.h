#pragma once

#include <cstdint>
#include <vector>

#include "esphome/core/component.h"
#include "esphome/components/cc1101/cc1101.h"

namespace esphome {
namespace rflink_universal {

// Runs before the SPI bus and CC1101 components. Its only job is to make the
// shared bus deterministic at power-up: all CS lines are driven HIGH before
// any radio can see SPI traffic, then every radio gets a non-destructive
// CHIP_RDYn/SO readiness probe.
class RadioBootGuard : public Component {
 public:
  void add_cs_pin(uint8_t pin) { this->cs_pins_.push_back(pin); }
  void add_gdo_pin(uint8_t pin) { this->gdo_pins_.push_back(pin); }

  // RX on GDO2: disconnect GDO0 output inside the CC1101 (IOCFG0=0x2E).
  // TX: reconnect async TX data input (IOCFG0=0x0D) once radio is in TX.
  void set_async_gdo0_rx_safe(cc1101::CC1101Component *radio);
  void set_async_gdo0_tx_data(cc1101::CC1101Component *radio);
  void set_sck_pin(uint8_t pin) { this->sck_pin_ = pin; this->has_sck_pin_ = true; }
  void set_mosi_pin(uint8_t pin) { this->mosi_pin_ = pin; this->has_mosi_pin_ = true; }
  void set_miso_pin(uint8_t pin) { this->miso_pin_ = pin; this->has_miso_pin_ = true; }
  void set_startup_delay_ms(uint32_t value) { this->startup_delay_ms_ = value; }
  void set_ready_timeout_us(uint32_t value) { this->ready_timeout_us_ = value; }
  void set_ready_retries(uint8_t value) { this->ready_retries_ = value; }
  void set_post_guard_delay_ms(uint32_t value) { this->post_guard_delay_ms_ = value; }

  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override;

 private:
  bool wait_chip_ready_(uint8_t cs_pin, uint8_t index);
  void configure_safe_bus_idle_();
  void configure_pre_power_gpio_inputs_();
  void write_async_gdo0_config_(cc1101::CC1101Component &radio, uint8_t value);
  bool check_miso_idle_high_(const char *where);
  void deselect_all_();

  std::vector<uint8_t> cs_pins_;
  std::vector<uint8_t> gdo_pins_;
  uint8_t sck_pin_{0};
  uint8_t mosi_pin_{0};
  uint8_t miso_pin_{0};
  bool has_sck_pin_{false};
  bool has_mosi_pin_{false};
  bool has_miso_pin_{false};
  uint32_t startup_delay_ms_{200};
  uint32_t ready_timeout_us_{10000};
  uint8_t ready_retries_{5};
  uint32_t post_guard_delay_ms_{25};
  std::vector<bool> ready_;
};

}  // namespace rflink_universal
}  // namespace esphome
