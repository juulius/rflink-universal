"""Stage 28: firmware diagnostics start before CC1101 hardware initialization.

Host-only lifecycle contracts. Actual Wi-Fi/OTA availability during a bootloop
requires ESP32-S3 hardware tests and is not claimed by this regression suite.
"""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
FW = ROOT / "firmware"


class EarlyDiagnosticsContracts(unittest.TestCase):
    def test_boot_order_starts_wifi_http_and_logs_before_radio(self):
        main = (FW / "main/main.cpp").read_text("utf-8")
        record = main.index("rflink::crash_report_record_boot();")
        init_logs = main.index("rflink_set_log_store_provider(&logs);")
        early_phase = main.index("BootPhase::starting_network")
        web = main.index("const bool web_ok=rflink_start_web_ota();")
        bus = main.index("e=bus.begin();")
        radio_phase = main.index("BootPhase::probing_radios")
        receivers = main.index("manager.start_receive(")
        publish = main.index("rflink_publish_radio_providers();")
        mqtt = main.index("rflink::mqtt_service_init();")
        self.assertLess(record, init_logs)
        self.assertLess(init_logs, early_phase)
        self.assertLess(early_phase, web)
        self.assertLess(web, bus)
        self.assertLess(bus, receivers)
        self.assertLess(radio_phase, receivers)
        self.assertLess(receivers, publish)
        self.assertLess(publish, mqtt)
        self.assertEqual(main.count("rflink_start_web_ota();"), 1)
        self.assertIn("rflink::BootPhase::verifying_ota", main)
        self.assertIn("esp_ota_mark_app_valid_cancel_rollback()", main)
        self.assertIn("esp_ota_mark_app_invalid_rollback_and_reboot()", main)

    def test_early_http_handlers_never_dereference_incomplete_radio(self):
        web = (FW / "main/rflink_web_ota.cpp").read_text("utf-8")
        header = (FW / "main/rflink_web_ota.hpp").read_text("utf-8")
        self.assertIn("void rflink_publish_radio_providers();", header)
        self.assertIn("radio_providers_published.store(true,std::memory_order_release)", web)
        self.assertIn("radio_providers_published.load(std::memory_order_acquire)", web)
        self.assertIn('"radio_initialized"', web.replace(r"\"", '"'))
        for name in ("rx_stats_provider", "ook_stats_a", "ook_stats_b",
                     "decoder_stats_provider", "manager_provider",
                     "spi_provider", "worker_cycles_provider"):
            self.assertIn(name, web)
        for expression in ("radio_ready && rx_stats_provider",
                           "radio_ready && ook_stats_a",
                           "radio_ready && ook_stats_b",
                           "radio_ready && decoder_stats_provider",
                           "radio_ready && manager_provider",
                           "radio_ready && spi_provider"):
            self.assertIn(expression, web)
        self.assertIn('"/api/diagnostics/crashes"', web)
        self.assertIn('"/api/logs"', web)
        self.assertIn('"/api/ota"', web)
        self.assertIn("native_ota_identity_accepted(", web)
        self.assertIn("same_board_profile_for_ota(", web)
        self.assertIn("startup_complete_published.load(std::memory_order_acquire)", web)
        self.assertIn("startup_complete_published.store(true,std::memory_order_release)", web)
        self.assertIn("void rflink_publish_startup_complete();", header)
        self.assertIn("rflink_publish_startup_complete();",
                      (FW / "main/main.cpp").read_text("utf-8"))

    def test_phase_numbers_stable_while_network_moves_first(self):
        source = r'''
#include "rflink_boot_phases.hpp"
#include <cassert>
using namespace rflink;
int main() {
  // Stage 27 flash entries keep their original numeric values.
  assert(static_cast<unsigned>(BootPhase::probing_radios) == 2);
  assert(static_cast<unsigned>(BootPhase::starting_receivers) == 3);
  assert(static_cast<unsigned>(BootPhase::starting_network) == 4);
  assert(kBootPhaseVersion == 1);
  BootPhaseHistory journal{};
  assert(boot_phase_begin(journal, 1));
  assert(boot_phase_set(journal, 1, BootPhase::starting_network));
  assert(boot_phase_find(journal, 1)->stage == 4);
  assert(boot_phase_set(journal, 1, BootPhase::probing_radios));
  assert(boot_phase_set(journal, 1, BootPhase::starting_receivers));
  assert(!boot_phase_set(journal, 1, BootPhase::starting_network));
  assert(boot_phase_set(journal, 1, BootPhase::app_ready, true));
  assert(boot_phase_begin(journal, 2));
  assert(boot_phase_find(journal, 1)->stage ==
         static_cast<uint8_t>(BootPhase::app_ready));
  // Old checkpoint IDs remain legible across firmware update.
  BootPhaseHistory old_history{};
  assert(boot_phase_begin(old_history, 1));
  old_history.entries[0].stage=4;
  assert(boot_phase_find(old_history, 1)->stage ==
         static_cast<uint8_t>(BootPhase::starting_network));
  return 0;
}
'''
        with tempfile.TemporaryDirectory() as d:
            src = Path(d) / "test.cpp"
            binary = Path(d) / "test"
            src.write_text(source, encoding="utf-8")
            check = subprocess.run(
                ["g++", "-std=c++17", "-O2", "-Wall", "-Wextra", "-Werror",
                 "-I" + str(FW / "main"), str(src), "-o", str(binary)],
                capture_output=True, text=True, timeout=40)
            self.assertEqual(check.returncode, 0, check.stderr)
            run = subprocess.run([str(binary)], capture_output=True,
                                 text=True, timeout=20)
            self.assertEqual(run.returncode, 0, run.stderr)

    def test_builder_reports_radio_pending_without_rebinding(self):
        proxy = (ROOT / "rflink_builder/diagnostic_proxy.py").read_text("utf-8")
        ui = (ROOT / "rflink_builder/static/rf_console.js").read_text("utf-8")
        self.assertIn('"radio_initialized"', proxy)
        self.assertIn('"startup_complete"', proxy)
        self.assertIn("radio_initialized===false", ui)
        self.assertIn("startup_complete===false", ui)
        devices=(ROOT / "rflink_builder/devices.py").read_text("utf-8")
        self.assertIn('if device.get("startup_complete") is False:', devices)
        self.assertIn('"startup_complete": info.get("startup_complete")', devices)
        self.assertIn("rfDiagCrashSignature", ui)
        self.assertIn("rfDiagCrashRetryAt=Date.now()", ui)
        self.assertIn("rfDiagCrashLoaded=true", ui)
        # No changes to the existing auto-binding / flash path.
        installer = (ROOT / "rflink_builder/static/rf_flash.js").read_text("utf-8")
        self.assertIn("rfFlashWaitForHardware(", installer)


if __name__ == "__main__":
    unittest.main()
