#pragma once
// Native port of the *electrical pre-SPI startup sequence* used in the
// user's working Radio Gaga v0.6.6 RadioBootGuard. No ESPHome dependency.
// This is only the first stage of full RX/TX/RMT parity, not the complete port.
#include <array>
#include "esp_err.h"

namespace rflink {
struct NativeBootGuardReport {
  esp_err_t gpio_error = ESP_OK;
  std::array<bool,3> chip_ready{{false,false,false}};
  bool miso_idle_high_before = false;
  bool miso_idle_high_after = false;
  bool gpio_setup_ok() const { return gpio_error == ESP_OK; }
};

// Call exactly once early in app_main before any SPI bus/device init, UART,
// RMT or CC1101 operations. High-impedance warmup is safe only before other
// tasks claim radio GPIOs. Missing modules are nonfatal; GPIO errors are not.
NativeBootGuardReport prepare_radio_pins_before_spi();
} // namespace rflink
