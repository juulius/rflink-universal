"""Onboard GPIO48 WS2812 native status LED host simulation and safety contracts.

This validates state priority, timing and wiring without claiming a physical
RGB LED or RF bench test. ESP-IDF / RMT compilation is checked separately in CI.
"""
import json
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
MAIN=ROOT/"firmware/main"


class NativeStatusLedTests(unittest.TestCase):
    def test_mode_priority_color_patterns_and_persistent_settings(self):
        code=r'''
#include "rflink_led_policy.hpp"
#include "rflink_radio_hal.hpp"
#include <cassert>
#include <cstring>
using namespace rflink;
int main() {
  static_assert(kStatusLedGpio==48);
  LedSettings settings{};
  assert(settings.brightness==20); // v0.6.6 Radio Gaga factory default
  assert(led_settings_valid(settings));
  settings.enabled=2;assert(!led_settings_valid(settings));
  settings.enabled=1;settings.brightness=0;assert(!led_settings_valid(settings));
  settings.brightness=101;assert(!led_settings_valid(settings));
  settings.brightness=12;assert(led_settings_valid(settings));
  LedInputs state{};
  assert(led_mode(settings,state)==LedMode::boot);
  state.startup_complete=true;
  assert(led_mode(settings,state)==LedMode::wifi_disconnected);
  state.wifi_connected=true;
  assert(led_mode(settings,state)==LedMode::radio_degraded);
  state.detected_radios=3;state.active_receivers=3;
  assert(led_mode(settings,state)==LedMode::healthy);
  auto green=led_sample(LedMode::healthy,0,12);
  assert(green.green>0 && green.red==0 && green.blue==0);
  auto dim=led_sample(LedMode::healthy,0,1);
  assert(dim.green>0 && dim.green<=green.green);
  // Exact timing from Radio Gaga v0.6.6 status_led.h::sample().
  // Two 130-ms pulses (0..129 and 235..364) every 6000 ms.
  // Its READY state stays faintly lit at 8% of selected brightness.
  auto idle=led_sample(LedMode::healthy,800,12);
  assert(idle.green>0 && idle.green<green.green);
  assert(idle.red==0 && idle.blue==0);
  assert(led_sample(LedMode::healthy,129,12).green==green.green);
  assert(led_sample(LedMode::healthy,130,12).green==idle.green);
  assert(led_sample(LedMode::healthy,234,12).green==idle.green);
  assert(led_sample(LedMode::healthy,235,12).green==green.green);
  assert(led_sample(LedMode::healthy,364,12).green==green.green);
  assert(led_sample(LedMode::healthy,365,12).green==idle.green);
  assert(led_sample(LedMode::healthy,5999,12).green==idle.green);
  assert(led_sample(LedMode::healthy,6000,12).green==green.green);
  assert(led_sample(LedMode::healthy,12000,12).green==green.green);
  state.ota_active=true;
  assert(led_mode(settings,state)==LedMode::ota);
  auto white=led_sample(LedMode::ota,0,12);
  assert(white.red==white.green && white.green==white.blue && white.blue>0);
  state.error=true;
  // Radio Gaga priority: OTA above Error, even while heartbeat is muted.
  assert(led_mode(settings,state)==LedMode::ota);
  state.ota_active=false;
  assert(led_mode(settings,state)==LedMode::error);
  auto red=led_sample(LedMode::error,0,12);
  assert(red.red>0 && red.green==0 && red.blue==0);
  settings.enabled=0;
  // The v0.6.6 "quiet" setting only mutes the healthy heartbeat.
  assert(led_mode(settings,state)==LedMode::error);
  state.error=false;
  assert(led_mode(settings,state)==LedMode::off);
  state.ota_active=true;
  assert(led_mode(settings,state)==LedMode::ota);
  state.ota_active=false;
  state.wifi_connected=false;
  assert(led_mode(settings,state)==LedMode::wifi_disconnected);
  state.wifi_connected=true;
  state.detected_radios=2;
  assert(led_mode(settings,state)==LedMode::radio_degraded);
  state.startup_complete=false;
  assert(led_mode(settings,state)==LedMode::boot);
  assert(std::strcmp(led_mode_name(LedMode::radio_degraded),"radio_degraded")==0);
  auto amber=led_sample(LedMode::radio_degraded,0,12);
  assert(amber.red>amber.green && amber.green>0 && amber.blue==0);
  auto blue=led_sample(LedMode::wifi_disconnected,0,12);
  assert(blue.blue>blue.green && blue.red==0);
  return 0;
}
'''
        with tempfile.TemporaryDirectory() as directory:
            file=Path(directory)/"led.cpp"
            exe=Path(directory)/"led_test"
            file.write_text(code)
            built=subprocess.run(["g++","-std=c++17","-Wall","-Wextra","-Werror",
                                  "-I"+str(MAIN),"-I"+str(ROOT/"firmware/components/rflink_radio_hal/include"),str(file),"-o",str(exe)],
                                 capture_output=True,text=True,timeout=35)
            self.assertEqual(built.returncode,0,built.stderr)
            run=subprocess.run([str(exe)],capture_output=True,text=True,timeout=15)
            self.assertEqual(run.returncode,0,run.stderr)

    def test_led_gpio_never_conflicts_with_radio_spi(self):
        board=json.loads((ROOT/"firmware/boards/rflink_s3_n16r8.json").read_text())
        pins=list(board["spi"][p] for p in ("sclk","mosi","miso"))
        for radio in board["radios"]:
            pins.extend(radio[p] for p in ("cs","gdo0","gdo2"))
        self.assertNotIn(48,pins)

    def test_led_is_bench_optional_and_not_ota_health_gate(self):
        main=(MAIN/"main.cpp").read_text()
        driver=(MAIN/"rflink_status_led.cpp").read_text()
        web=(MAIN/"rflink_web_ota.cpp").read_text()
        self.assertLess(main.index("rflink_publish_radio_providers();"),
                        main.index("status_led_start();"))
        self.assertIn("update_led_receiver_status();",main)
        self.assertIn("status_led_set_wifi_connected(false)",web)
        self.assertIn("status_led_set_wifi_connected(true)",web)
        # A failed native OTA health check must not masquerade as green.
        self.assertIn("rflink::status_led_signal_error();",main)
        self.assertIn("rflink::status_led_set_startup_complete(true);",main)
        self.assertIn("status_led_set_radios(detected,active)",main)
        self.assertIn("status_led_set_ota_active(true)",web)
        self.assertIn("status_led_set_ota_active(false)",web)
        self.assertIn("status_led_signal_error()",web)
        self.assertIn("rmt_new_tx_channel",driver)
        self.assertNotIn("gpio_reset_pin",driver)
        self.assertNotIn("esp_ota_mark_app_valid",driver)
        self.assertNotIn("esp_ota_mark_app_invalid",driver)
        self.assertNotIn("stop_receive",driver)
        self.assertNotIn("start_receive",driver)
        self.assertIn("GRB",driver)
        self.assertIn("mem_block_symbols=48",driver)
        self.assertIn("rmt_tx_wait_all_done(channel,100)",driver)
        self.assertIn("vTaskDelay(pdMS_TO_TICKS(100))",driver)
        self.assertIn("status_led_set_radios(detected,active)",main)
        self.assertIn('"rf_led"',driver)
        # The LED driver remains initialized so errors can interrupt even
        # when the normal heartbeat is disabled by the user.
        self.assertIn("if(!attempted){",driver)
        self.assertNotIn("if(settings.enabled && !attempted)",driver)

    def test_live_status_reports_led_state_without_touching_radio_metrics(self):
        web=(MAIN/"rflink_web_ota.cpp").read_text()
        for key in ('led_gpio','led_enabled','led_brightness',
                    'led_driver_ready','led_mode'):
            self.assertIn(key,web)
        self.assertIn('rflink::led_mode_name(led.mode)',web)
        self.assertIn('spi_ready_timeouts',web)
        self.assertIn('rx43342_listening',web)
        self.assertIn('rx43392_listening',web)
        self.assertIn('rx868_listening',web)
        self.assertIn('rflink_status_led.cpp', (MAIN/"CMakeLists.txt").read_text())

    def test_local_led_settings_are_keyless_but_ota_remains_authorized(self):
        web=(MAIN/"rflink_web_ota.cpp").read_text()
        form=(MAIN/"webui/index.html").read_text()
        script=(MAIN/"webui/app.js").read_text()
        endpoint=web[web.index("static esp_err_t led_config_post"):web.index("static esp_err_t events_get")]
        self.assertIn("if(!browser_json_write(req))",endpoint)
        self.assertNotIn("if(!key_ok(req))",endpoint)
        self.assertIn("if(ota_busy.load())",endpoint)
        self.assertIn("status_led_save_settings(candidate)",endpoint)
        self.assertIn("LED settings not persisted; unchanged",endpoint)
        self.assertIn("nvs_commit(handle)",(MAIN/"rflink_status_led.cpp").read_text())
        self.assertIn('led_get.uri="/api/led"',web)
        self.assertIn('led_post.uri="/api/led"',web)
        self.assertNotIn('id="led-key"',form)
        self.assertNotIn("'X-RFLink-Update-Key':key",script)
        self.assertIn("if(!key_ok(req))",web[web.index("static esp_err_t ota_post"):])
        self.assertIn("api('/api/led',{method:'POST'",script)


if __name__=="__main__":
    unittest.main()
