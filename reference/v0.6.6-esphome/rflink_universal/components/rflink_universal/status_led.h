#pragma once

// Pure, host-testable LED policy; hardware rendering stays in the ESPHome YAML.
#include <cstdint>

namespace esphome {
namespace rflink_universal {

class StatusLed {
 public:
  enum class Mode : uint8_t {
    BOOT, CONNECTING, READY, OFFLINE, LEARNING, CAPTURE, LEARNED,
    PAIRING, TX, OTA, RADIO_MISSING, ERROR
  };
  struct Pixel {
    float red{0};
    float green{0};
    float blue{0};
    float brightness{0};
  };

  void boot(uint32_t now) { boot_until_ = now + 3000; }
  void wifi(bool online) { wifi_known_ = true; wifi_online_ = online; }
  void ota(bool active) { ota_ = active; }
  void fault(bool active) { fault_ = active; }
  void radio_missing(bool missing) { radio_missing_ = missing; }
  bool has_missing_radio() const { return radio_missing_; }
  void error(uint32_t now) { error_since_ = now; error_until_ = now + 3500; }
  void learn(bool active) {
    learning_ = active;
    // A successful confirmation must remain visible briefly even when the
    // user immediately stops listening. Only clear an unfinished RX flash.
    if (!active && event_ == Event::CAPTURE) event_ = Event::NONE;
  }
  bool learning() const { return learning_; }
  void capture(uint32_t now) {
    if (learning_) { event_ = Event::CAPTURE; event_since_ = now; event_until_ = now + 700; }
  }
  void learned(uint32_t now) {
    event_ = Event::LEARNED; event_since_ = now; event_until_ = now + 1350;
  }
  // Pairing is an outgoing command, not proof that the target device paired.
  // Expire even if the send script aborts before calling pair_end().
  void pair_start(uint32_t now) { pair_until_ = now + 15000; }
  void pair_end(uint32_t now) { pair_until_ = now + 900; }
  void tx(uint32_t now) { tx_until_ = now + 180; }

  void settings(uint8_t brightness, bool quiet) {
    brightness_ = brightness < 1 ? 1 : (brightness > 100 ? 100 : brightness);
    quiet_ = quiet;
  }
  uint8_t brightness() const { return brightness_; }
  bool quiet() const { return quiet_; }

  Mode mode(uint32_t now) const {
    if (ota_) return Mode::OTA;
    if (fault_ || live_(error_until_, now)) return Mode::ERROR;
    if (event_ == Event::LEARNED && live_(event_until_, now)) return Mode::LEARNED;
    if (live_(pair_until_, now)) return Mode::PAIRING;
    if (learning_) {
      if (event_ == Event::CAPTURE && live_(event_until_, now)) return Mode::CAPTURE;
      return Mode::LEARNING;
    }
    if (live_(boot_until_, now)) return Mode::BOOT;
    if (!wifi_known_) return Mode::CONNECTING;
    if (!wifi_online_) return Mode::OFFLINE;
    if (radio_missing_) return Mode::RADIO_MISSING;
    if (live_(tx_until_, now)) return Mode::TX;
    return Mode::READY;
  }

  static const char *name(Mode mode) {
    switch (mode) {
      case Mode::BOOT: return "opstarten";
      case Mode::CONNECTING: return "wifi_verbinden";
      case Mode::READY: return "gereed";
      case Mode::OFFLINE: return "verbinding_verloren";
      case Mode::RADIO_MISSING: return "radio_ontbreekt";
      case Mode::LEARNING: return "inleren";
      case Mode::CAPTURE: return "signaal_ontvangen";
      case Mode::LEARNED: return "apparaat_bevestigd";
      case Mode::PAIRING: return "koppelopdracht";
      case Mode::TX: return "rf_verzenden";
      case Mode::OTA: return "ota";
      case Mode::ERROR: return "fout";
    }
    return "onbekend";
  }

  // Stable machine-readable status for APIs/UI. Keep name() for backward
  // compatibility with older clients that may still consume the Dutch token.
  static const char *code(Mode mode) {
    switch (mode) {
      case Mode::BOOT: return "boot";
      case Mode::CONNECTING: return "connecting";
      case Mode::READY: return "ready";
      case Mode::OFFLINE: return "offline";
      case Mode::RADIO_MISSING: return "radio_missing";
      case Mode::LEARNING: return "learning";
      case Mode::CAPTURE: return "capture";
      case Mode::LEARNED: return "learned";
      case Mode::PAIRING: return "pairing";
      case Mode::TX: return "tx";
      case Mode::OTA: return "ota";
      case Mode::ERROR: return "error";
    }
    return "unknown";
  }

  Pixel sample(uint32_t now) const {
    const Mode m = mode(now);
    Pixel p;
    const float max = brightness_ / 100.0f;
    switch (m) {
      case Mode::BOOT: p.blue=1; p.brightness=max * pulse_(now, 1100); break;
      case Mode::CONNECTING: p.red=1; p.green=0.75f; p.brightness=max * pulse_(now, 1700); break;
      case Mode::READY: {
        if (quiet_) break;
        const uint32_t phase=now%6000;
        const bool beat = (phase<130 || (phase>=235 && phase<365));
        p.green=1; p.brightness=beat ? max : max*0.08f;
        break;
      }
      case Mode::RADIO_MISSING: {
        // Warning only: the network and OTA remain operational. Two amber
        // flashes every 3 seconds distinguish missing hardware from fatal red.
        const uint32_t phase=now%3000;
        const bool beat=(phase<210 || (phase>=350 && phase<560));
        p.red=1; p.green=0.34f; p.brightness=beat ? max : 0;
        break;
      }
      case Mode::OFFLINE: {
        const uint32_t phase=now%2500;
        const bool beat=(phase<170 || (phase>=300 && phase<470));
        p.red=1; p.green=0.40f; p.brightness=beat ? max : 0;
        break;
      }
      case Mode::LEARNING: p.green=0.85f; p.blue=1; p.brightness=max*pulse_(now, 1200); break;
      case Mode::CAPTURE: {
        const uint32_t phase=now-event_since_;
        p.green=1; p.blue=0.7f;
        p.brightness=(phase<145 || (phase>=260 && phase<405)) ? max : 0;
        break;
      }
      case Mode::LEARNED: {
        const uint32_t phase=now-event_since_;
        p.green=1;
        p.brightness=(phase<160 || (phase>=290 && phase<450) || (phase>=580 && phase<740)) ? max : 0;
        break;
      }
      case Mode::PAIRING: p.red=0.64f; p.blue=1; p.brightness=max*pulse_(now, 1100); break;
      case Mode::TX: p.blue=1; p.brightness=max; break;
      case Mode::OTA: p.red=p.green=p.blue=1; p.brightness=max*pulse_(now, 1300); break;
      case Mode::ERROR: {
        const uint32_t phase=(now-error_since_)%1400;
        p.red=1;
        p.brightness=(phase<135 || (phase>=250 && phase<385) || (phase>=500 && phase<635)) ? max : 0;
        break;
      }
    }
    return p;
  }

 private:
  enum class Event : uint8_t { NONE, CAPTURE, LEARNED };
  static bool live_(uint32_t until, uint32_t now) {
    return until != 0 && static_cast<int32_t>(until-now) > 0;
  }
  static float pulse_(uint32_t now, uint32_t period) {
    const float progress = static_cast<float>(now%period)/(period/2.0f);
    return 0.08f + 0.92f*(progress <= 1.0f ? progress : 2.0f-progress);
  }
  bool wifi_known_{false};
  bool wifi_online_{false};
  bool ota_{false};
  bool fault_{false};
  bool radio_missing_{false};
  bool learning_{false};
  bool quiet_{false};
  uint8_t brightness_{20};
  Event event_{Event::NONE};
  uint32_t boot_until_{0};
  uint32_t pair_until_{0};
  uint32_t tx_until_{0};
  uint32_t event_since_{0};
  uint32_t event_until_{0};
  uint32_t error_since_{0};
  uint32_t error_until_{0};
};

}  // namespace rflink_universal
}  // namespace esphome
