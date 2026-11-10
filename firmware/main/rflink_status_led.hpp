#pragma once
// Native WS2812 status indicator. No dependency on ESPHome or RF transmit APIs.
#include <cstdint>
#include "rflink_led_policy.hpp"
#include "rflink_radio_hal.hpp"
namespace rflink {
struct LedSnapshot {
  LedSettings settings{};
  LedMode mode=LedMode::off;
  bool driver_ready=false;
  uint32_t frames_sent=0;
  uint32_t errors=0;
  int32_t last_error=0;
};
bool status_led_start(); // call AFTER radio RMT receivers claim their resources
void status_led_set_wifi_connected(bool connected);
void status_led_set_radios(unsigned detected,unsigned active_receivers);
void status_led_set_startup_complete(bool complete);
void status_led_set_ota_active(bool active);
void status_led_signal_error(); // bounded 10-second red indicator
LedSnapshot status_led_snapshot();
bool status_led_save_settings(const LedSettings& settings); // commit before apply
} // namespace rflink
