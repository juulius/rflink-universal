#pragma once
// Standalone, host-testable WS2812 status policy (no ESP-IDF dependencies).
// Radio presence and listening are separate. Never indicate healthy RF just
// because the web server responds or a CC1101 probe returned a PARTNUM.
#include <cstdint>

namespace rflink {
constexpr uint32_t kLedSettingsMagic=0x52464c44; // RFLD
struct LedSettings {
  uint32_t magic=kLedSettingsMagic;
  uint8_t version=1;
  uint8_t enabled=1;
  uint8_t brightness=20; // v0.6.6 Radio Gaga default; NVS preferences take precedence
  uint8_t reserved=0;
};
inline bool led_settings_valid(const LedSettings& s) {
  return s.magic==kLedSettingsMagic && s.version==1 && s.enabled<=1 &&
         s.brightness>=1 && s.brightness<=100 && s.reserved==0;
}
enum class LedMode : uint8_t {
  off=0, boot=1, wifi_disconnected=2, radio_degraded=3,
  healthy=4, ota=5, error=6
};
inline const char* led_mode_name(LedMode mode) {
  switch(mode) {
    case LedMode::off:return "off";
    case LedMode::boot:return "boot";
    case LedMode::wifi_disconnected:return "wifi_disconnected";
    case LedMode::radio_degraded:return "radio_degraded";
    case LedMode::healthy:return "healthy";
    case LedMode::ota:return "ota";
    case LedMode::error:return "error";
    default:return "unknown";
  }
}
struct LedInputs {
  bool startup_complete=false;
  bool wifi_connected=false;
  uint8_t detected_radios=0;
  uint8_t active_receivers=0;
  bool ota_active=false;
  bool error=false;
};
struct LedRgb { uint8_t red=0,green=0,blue=0; };
// ESPHome-style gamma 2.8 for the user's 1..100% *perceived* brightness.
// Precomputed 8-bit LUT: no floating-point powf in the 100ms LED task.
// Clamp nonzero levels to one WS2812 code point so 1-15% never vanish.
constexpr uint8_t kLedGamma28[101]={
  0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 4, 4,
  5, 5, 6, 7, 7, 8, 9, 10, 10, 11, 12, 13,
  15, 16, 17, 18, 20, 21, 22, 24, 26, 27, 29, 31,
  33, 35, 37, 39, 41, 43, 45, 48, 50, 53, 55, 58,
  61, 64, 67, 70, 73, 76, 80, 83, 87, 90, 94, 98,
  102, 106, 110, 114, 118, 123, 127, 132, 137, 141, 146, 151,
  157, 162, 167, 173, 178, 184, 190, 196, 202, 208, 214, 221,
  227, 234, 241, 248, 255
};
inline uint8_t led_gamma_brightness(uint8_t percent) {
  return kLedGamma28[percent<=100?percent:100];
}
inline uint8_t led_scale_channel(uint8_t channel,uint8_t level) {
  // Preserve the intended RGB channel proportions, with exact zeroes.
  return static_cast<uint8_t>((unsigned(channel)*level+127u)/255u);
}

inline LedMode led_mode(const LedSettings& settings,const LedInputs& in) {
  // v0.6.6 Radio Gaga's "quiet" setting mutes READY only: network,
  // radio, boot, error and OTA indications are never silenced by it.
  // OTA takes precedence over error, as in status_led.h::mode().
  if(in.ota_active)return LedMode::ota;
  if(in.error)return LedMode::error;
  if(!in.startup_complete)return LedMode::boot;
  if(!in.wifi_connected)return LedMode::wifi_disconnected;
  if(in.detected_radios!=3 || in.active_receivers!=3)return LedMode::radio_degraded;
  return settings.enabled?LedMode::healthy:LedMode::off;
}
inline LedRgb led_sample(LedMode mode,uint32_t uptime_ms,uint8_t brightness) {
  if(!brightness)return {};
  bool on=false;
  LedRgb full{};
  // Warnings are not gated by the green heartbeat toggle and must remain
  // visible even if the user uses 1% for a bedroom/quiet installation.
  constexpr uint8_t kWarningMinPwm=8;
  const auto normal_level=led_gamma_brightness(brightness);
  switch(mode) {
    case LedMode::boot:
      on=uptime_ms%600<250;full={0,35,255};break; // blue start
    case LedMode::wifi_disconnected: {
      const auto phase=uptime_ms%1600;
      on=phase<100 || (phase>=230 && phase<330);
      full={0,65,255};break; // double blue: network missing
    }
    case LedMode::radio_degraded: {
      const auto phase=uptime_ms%1600;
      on=phase<160 || (phase>=300 && phase<460);
      full={255,95,0};break; // amber: missing/unresponsive RX
    }
    case LedMode::healthy: {
      // Preserve Radio Gaga's two short pulses in each 6000ms cycle.
      // A code point of 1 cannot also represent an 8%-of-1 background
      // glow. In that case it is genuinely off between heartbeats.
      const auto phase=uptime_ms%6000;
      const bool beat=phase<130 || (phase>=235 && phase<365);
      const unsigned level=beat?normal_level:
        (normal_level>=3 ? (normal_level*8u+99u)/100u : 0u);
      return {0,static_cast<uint8_t>(level),0};
    }
    case LedMode::ota:
      on=uptime_ms%520<260;full={255,255,255};break; // white during OTA
    case LedMode::error:
      on=uptime_ms%420<210;full={255,0,0};break; // red
    case LedMode::off:default:break;
  }
  if(!on)return {};
  const auto level=normal_level<kWarningMinPwm?kWarningMinPwm:normal_level;
  return {led_scale_channel(full.red,level),
          led_scale_channel(full.green,level),
          led_scale_channel(full.blue,level)};
}
} // namespace rflink
