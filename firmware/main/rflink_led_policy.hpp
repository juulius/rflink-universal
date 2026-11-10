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
  uint8_t brightness=12; // percentage; deliberately dim by default
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
inline LedMode led_mode(const LedSettings& settings,const LedInputs& in) {
  // "Enabled" controls normal heartbeat/boot animations only. Important
  // faults and OTA state must remain visible even when heartbeat is disabled
  // (the user-visible behavior expected from the v0.6 RFLink status light).
  if(in.error)return LedMode::error;
  if(in.ota_active)return LedMode::ota;
  if(!in.startup_complete)return settings.enabled?LedMode::boot:LedMode::off;
  if(!in.wifi_connected)return LedMode::wifi_disconnected;
  if(in.detected_radios!=3 || in.active_receivers!=3)return LedMode::radio_degraded;
  return settings.enabled?LedMode::healthy:LedMode::off;
}
inline LedRgb led_sample(LedMode mode,uint32_t uptime_ms,uint8_t brightness) {
  if(!brightness)return {};
  bool on=false;
  LedRgb full{};
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
      // Quiet green double heartbeat: two short, dim pulses in a 5 s
      // activity window, then at least 10 s completely dark (15 s cycle).
      // Unlike the old v0.7 single flash every 2 s, this remains subtle.
      const auto phase=uptime_ms%15000;
      on=phase<220 || (phase>=450 && phase<670);
      full={0,255,0};break;
    }
    case LedMode::ota:
      on=uptime_ms%520<260;full={255,255,255};break; // white during OTA
    case LedMode::error:
      on=uptime_ms%420<210;full={255,0,0};break; // red
    case LedMode::off:default:break;
  }
  if(!on)return {};
  // Round so 1-3% settings stay visibly nonzero, but always bounded.
  const auto scale=[brightness](uint8_t v)->uint8_t{
    return static_cast<uint8_t>((unsigned(v)*brightness+99)/100);
  };
  return {scale(full.red),scale(full.green),scale(full.blue)};
}
} // namespace rflink
