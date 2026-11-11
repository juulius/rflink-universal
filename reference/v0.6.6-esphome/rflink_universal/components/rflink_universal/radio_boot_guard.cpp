#include "radio_boot_guard.h"

#include "esphome/core/log.h"

#ifdef USE_ESP32
#include "driver/gpio.h"
#endif

namespace esphome::rflink_universal {

static const char *const TAG_RADIO_BOOT = "rflink.radio_boot";

// Run after any explicit power-supply component, but well before the SPI bus.
// This gives us the earliest useful application-level point to force all shared
// SPI lines into a deterministic idle state.
float RadioBootGuard::get_setup_priority() const { return setup_priority::POWER - 1.0f; }

void RadioBootGuard::deselect_all_() {
#ifdef USE_ESP32
  for (uint8_t pin : this->cs_pins_) {
    gpio_set_level(static_cast<gpio_num_t>(pin), 1);
  }
#endif
}

void RadioBootGuard::configure_pre_power_gpio_inputs_() {
#ifdef USE_ESP32
  // Before 3.3V radio rail has settled, do not drive any radio pin HIGH from
  // the ESP32's already-powered GPIO supply. The PCB's external CSn pull-ups
  // must be connected to the RADIO 3.3V rail, NOT the ESP32 3.3V rail.
  // This protects only the early setup window; a failed 3.3V rail still needs
  // a power-good/supervisor circuit for guaranteed electrical isolation.
  for (uint8_t pin : this->cs_pins_) {
    auto gpio_pin = static_cast<gpio_num_t>(pin);
    gpio_set_direction(gpio_pin, GPIO_MODE_INPUT);
    gpio_set_pull_mode(gpio_pin, GPIO_FLOATING);
  }
  if (this->has_sck_pin_) {
    auto gpio_pin = static_cast<gpio_num_t>(this->sck_pin_);
    gpio_set_direction(gpio_pin, GPIO_MODE_INPUT);
    gpio_set_pull_mode(gpio_pin, GPIO_FLOATING);
  }
  if (this->has_mosi_pin_) {
    auto gpio_pin = static_cast<gpio_num_t>(this->mosi_pin_);
    gpio_set_direction(gpio_pin, GPIO_MODE_INPUT);
    gpio_set_pull_mode(gpio_pin, GPIO_FLOATING);
  }
  if (this->has_miso_pin_) {
    auto gpio_pin = static_cast<gpio_num_t>(this->miso_pin_);
    gpio_set_direction(gpio_pin, GPIO_MODE_INPUT);
    gpio_set_pull_mode(gpio_pin, GPIO_FLOATING);
  }
  for (uint8_t pin : this->gdo_pins_) {
    auto gpio_pin = static_cast<gpio_num_t>(pin);
    gpio_set_direction(gpio_pin, GPIO_MODE_INPUT);
    gpio_set_pull_mode(gpio_pin, GPIO_FLOATING);
  }
#endif
}

// IOCFG0 (0x02): 0x2E disables the GDO0 output; 0x0D selects the
// asynchronous serial data function (RX output / TX input). In dual-pin
// wiring RX is via GDO2, so the CC1101 GDO0 output must be disabled during RX.
// Call only after the radio SPI bus is initialised, and NEVER on the 868 IRQ.
void RadioBootGuard::write_async_gdo0_config_(cc1101::CC1101Component &radio, uint8_t value) {
  if (radio.is_failed() || !radio.spi_is_ready()) return;
  radio.enable();
  radio.write_byte(0x02);  // CC1101 IOCFG0 register, write operation
  radio.write_byte(value);
  radio.disable();
}

void RadioBootGuard::set_async_gdo0_rx_safe(cc1101::CC1101Component *radio) {
  // ESPHome's id(cc1101_...) yields a pointer; skip absent devices safely.
  if (radio == nullptr) return;
  this->write_async_gdo0_config_(*radio, 0x2E);
}

void RadioBootGuard::set_async_gdo0_tx_data(cc1101::CC1101Component *radio) {
  // ESPHome's id(cc1101_...) yields a pointer; skip absent devices safely.
  if (radio == nullptr) return;
  this->write_async_gdo0_config_(*radio, 0x0D);
}

void RadioBootGuard::configure_safe_bus_idle_() {
#ifdef USE_ESP32
  // CC1101 CSn is active LOW. Preload HIGH, enable the ESP32's weak pull-up,
  // then switch the pin to output. The pull-up is intentionally left enabled;
  // it also gives us a defined inactive level while the GPIO matrix changes
  // ownership during later ESPHome SPI setup.
  for (uint8_t pin : this->cs_pins_) {
    const auto gpio_pin = static_cast<gpio_num_t>(pin);
    gpio_set_level(gpio_pin, 1);
    gpio_set_pull_mode(gpio_pin, GPIO_PULLUP_ONLY);
    gpio_set_direction(gpio_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(gpio_pin, 1);
  }

  // CC1101 uses SPI mode 0: SCK idles LOW. Keep SCK and MOSI quiet before any
  // CSn line is ever pulled LOW by the readiness probe. This avoids a floating
  // pre-SPI clock being interpreted as random header bits by a selected radio.
  if (this->has_sck_pin_) {
    const auto gpio_pin = static_cast<gpio_num_t>(this->sck_pin_);
    gpio_set_level(gpio_pin, 0);
    gpio_set_pull_mode(gpio_pin, GPIO_PULLDOWN_ONLY);
    gpio_set_direction(gpio_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(gpio_pin, 0);
  }
  if (this->has_mosi_pin_) {
    const auto gpio_pin = static_cast<gpio_num_t>(this->mosi_pin_);
    gpio_set_level(gpio_pin, 0);
    gpio_set_pull_mode(gpio_pin, GPIO_PULLDOWN_ONLY);
    gpio_set_direction(gpio_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(gpio_pin, 0);
  }

  // SO/MISO is tri-stated while every CSn is HIGH. A weak pull-up makes an
  // un-driven/missing radio read HIGH instead of a random level. Once a valid
  // CC1101 is selected it actively pulls SO LOW to signal CHIP_RDYn.
  if (this->has_miso_pin_) {
    const auto gpio_pin = static_cast<gpio_num_t>(this->miso_pin_);
    gpio_set_direction(gpio_pin, GPIO_MODE_INPUT);
    gpio_set_pull_mode(gpio_pin, GPIO_PULLUP_ONLY);
  }
#endif
}

bool RadioBootGuard::check_miso_idle_high_(const char *where) {
#ifdef USE_ESP32
  if (!this->has_miso_pin_) return true;
  delayMicroseconds(10);
  const bool high = gpio_get_level(static_cast<gpio_num_t>(this->miso_pin_)) != 0;
  if (!high) {
    ESP_LOGW(TAG_RADIO_BOOT,
             "Shared MISO/SO is LOW with all CSn HIGH (%s). Possible bridge, stuck slave or CSn not actually high",
             where);
  }
  return high;
#else
  (void) where;
  return true;
#endif
}

bool RadioBootGuard::wait_chip_ready_(uint8_t cs_pin, uint8_t index) {
#ifdef USE_ESP32
  if (!this->has_miso_pin_) return false;

  for (uint8_t attempt = 1; attempt <= this->ready_retries_; attempt++) {
    this->deselect_all_();
    if (this->has_sck_pin_) gpio_set_level(static_cast<gpio_num_t>(this->sck_pin_), 0);
    if (this->has_mosi_pin_) gpio_set_level(static_cast<gpio_num_t>(this->mosi_pin_), 0);
    delayMicroseconds(10);

    // Per the CC1101 SPI contract, after CSn goes LOW the MCU waits until SO
    // goes LOW before issuing the first clock/header bit.
    gpio_set_level(static_cast<gpio_num_t>(cs_pin), 0);
    const uint32_t started = micros();
    bool ready = false;
    while (static_cast<uint32_t>(micros() - started) < this->ready_timeout_us_) {
      if (gpio_get_level(static_cast<gpio_num_t>(this->miso_pin_)) == 0) {
        ready = true;
        break;
      }
      delayMicroseconds(20);
    }
    gpio_set_level(static_cast<gpio_num_t>(cs_pin), 1);

    // With the selected radio released, MISO should return HIGH via the weak
    // pull-up. If it remains LOW we log bus contention but continue; diagnostics
    // and the watchdog remain non-fatal by design.
    delayMicroseconds(10);
    if (gpio_get_level(static_cast<gpio_num_t>(this->miso_pin_)) == 0) {
      ESP_LOGW(TAG_RADIO_BOOT, "Radio %u did not release MISO/SO after CSn HIGH", index + 1);
    }

    if (ready) {
      ESP_LOGI(TAG_RADIO_BOOT, "Radio %u CHIP_RDYn ready on attempt %u/%u", index + 1, attempt,
               this->ready_retries_);
      return true;
    }

    ESP_LOGW(TAG_RADIO_BOOT, "Radio %u CHIP_RDYn timeout on attempt %u/%u", index + 1, attempt,
             this->ready_retries_);
    delay(5);
  }
#else
  (void) cs_pin;
  (void) index;
#endif
  return false;
}

void RadioBootGuard::setup() {
  // During the first 200 ms after application setup begins, all GPIOs going
  // to radio modules are high-impedance. External CSn pull-ups on the radio
  // supply keep an unpowered CC1101 deselected without ESP back-powering it.
  this->configure_pre_power_gpio_inputs_();
  ESP_LOGI(TAG_RADIO_BOOT, "Radio rails settling: GPIO inputs floating for %u ms", static_cast<unsigned>(this->startup_delay_ms_));
  if (this->startup_delay_ms_ != 0) delay(this->startup_delay_ms_);

  // Only after the rail-settle period, claim pins and probe each radio.
  this->configure_safe_bus_idle_();
  this->deselect_all_();
  ESP_LOGI(TAG_RADIO_BOOT,
           "Pre-SPI guard active: CSn=HIGH+pull-up, SCK=LOW+pull-down, MOSI=LOW+pull-down, MISO=input+pull-up");

  this->check_miso_idle_high_("before readiness probes");

  this->ready_.assign(this->cs_pins_.size(), false);
  for (size_t index = 0; index < this->cs_pins_.size(); index++) {
    this->ready_[index] = this->wait_chip_ready_(this->cs_pins_[index], static_cast<uint8_t>(index));
  }

  this->deselect_all_();
#ifdef USE_ESP32
  if (this->has_sck_pin_) gpio_set_level(static_cast<gpio_num_t>(this->sck_pin_), 0);
  if (this->has_mosi_pin_) gpio_set_level(static_cast<gpio_num_t>(this->mosi_pin_), 0);
#endif
  this->check_miso_idle_high_("after readiness probes");

  // Deliberate quiet gap before ESPHome claims/configures the hardware SPI bus.
  if (this->post_guard_delay_ms_ != 0) delay(this->post_guard_delay_ms_);

  unsigned ready_count = 0;
  for (bool ready : this->ready_) ready_count += ready ? 1U : 0U;
  ESP_LOGI(TAG_RADIO_BOOT,
           "CC1101 boot guard complete: %u/%u radios asserted CHIP_RDYn; continuing with ESPHome SPI setup",
           ready_count, static_cast<unsigned>(this->ready_.size()));
  // Never mark this component failed. Missing/marginal radios are optional and
  // the normal CC1101 components plus RFLink diagnostics decide availability.
}

void RadioBootGuard::dump_config() {
  ESP_LOGCONFIG(TAG_RADIO_BOOT, "RFLink CC1101 pre-SPI boot guard:");
  ESP_LOGCONFIG(TAG_RADIO_BOOT, "  CS lines: %u", static_cast<unsigned>(this->cs_pins_.size()));
  ESP_LOGCONFIG(TAG_RADIO_BOOT, "  GDO lines released during startup: %u", static_cast<unsigned>(this->gdo_pins_.size()));
  ESP_LOGCONFIG(TAG_RADIO_BOOT, "  SCK pin: %u", this->sck_pin_);
  ESP_LOGCONFIG(TAG_RADIO_BOOT, "  MOSI pin: %u", this->mosi_pin_);
  ESP_LOGCONFIG(TAG_RADIO_BOOT, "  MISO pin: %u", this->miso_pin_);
  ESP_LOGCONFIG(TAG_RADIO_BOOT, "  Pre-SPI settle delay: %u ms", static_cast<unsigned>(this->startup_delay_ms_));
  ESP_LOGCONFIG(TAG_RADIO_BOOT, "  CHIP_RDYn timeout: %u us", static_cast<unsigned>(this->ready_timeout_us_));
  ESP_LOGCONFIG(TAG_RADIO_BOOT, "  Attempts per radio: %u", this->ready_retries_);
  ESP_LOGCONFIG(TAG_RADIO_BOOT, "  Post-guard quiet delay: %u ms", static_cast<unsigned>(this->post_guard_delay_ms_));
}

}  // namespace esphome::rflink_universal
