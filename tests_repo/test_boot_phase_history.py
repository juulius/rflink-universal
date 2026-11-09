"""Stage 27: durable startup milestones and safe crash-history presentation.

Host-only tests. Does NOT claim a physical ESP32-S3 watchdog or NVS test.
"""
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
FW = ROOT / "firmware"


class BootPhaseHistoryTests(unittest.TestCase):
    def test_ring_order_and_invalid_or_nonmonotonic_stages(self):
        code = r'''
#include "rflink_boot_phases.hpp"
#include <cassert>
#include <cstring>
using namespace rflink;
int main() {
  BootPhaseHistory journal{};
  assert(boot_phase_history_valid(journal));
  assert(!boot_phase_begin(journal, 0));
  for (uint32_t boot=1; boot<=24; ++boot) {
    assert(boot_phase_begin(journal, boot));
    assert(boot_phase_find(journal, boot)->stage == 1);
    assert(boot_phase_set(journal, boot, BootPhase::probing_radios));
    assert(!boot_phase_set(journal, boot, BootPhase::probing_radios));
    assert(!boot_phase_set(journal, boot, BootPhase::nvs_ready));
    assert(!boot_phase_set(journal, boot, BootPhase::app_ready, false));
    // The ready phase was already set by the call above. Retrying with
    // startup_complete=true finishes the boot exactly once.
    assert(boot_phase_set(journal, boot, BootPhase::app_ready, true));
    assert(!boot_phase_set(journal, boot, BootPhase::app_ready, true));
    assert(!boot_phase_set(journal, boot, BootPhase::starting_network));
    const auto* entry = boot_phase_find(journal, boot);
    assert(entry && entry->startup_complete);
  }
  assert(journal.count == kBootPhaseCapacity);
  assert(boot_phase_find(journal, 24));
  assert(!boot_phase_find(journal, 12));
  assert(boot_phase_find(journal, 13));
  assert(!boot_phase_set(journal, 23, BootPhase::starting_network));
  assert(!boot_phase_set(journal, 24, BootPhase::unknown));
  assert(!boot_phase_set(journal, 24, BootPhase::verifying_ota, true));

  // Simulate a reboot interrupted while radio RX was starting.
  assert(boot_phase_begin(journal, 25));
  assert(boot_phase_set(journal, 25, BootPhase::starting_receivers));
  assert(boot_phase_begin(journal, 26));
  const auto* interrupted = boot_phase_find(journal, 25);
  assert(interrupted && interrupted->stage ==
         static_cast<uint8_t>(BootPhase::starting_receivers));
  assert(!interrupted->startup_complete);
  assert(std::strcmp(boot_phase_name(BootPhase::starting_receivers),
                     "starting_receivers") == 0);

  // New history after corrupted metadata or reset-history migration.
  journal.magic = 0;
  assert(boot_phase_begin(journal, 1));
  assert(journal.count == 1 && journal.last_sequence == 1);
  assert(!boot_phase_find(journal, 26));
  assert(boot_phase_find(journal, 1));

  // Sequence wrap in the existing reset ring must start a fresh journal.
  journal.last_sequence = UINT32_MAX;
  assert(boot_phase_begin(journal, 1));
  assert(journal.count == 1);
  return 0;
}
'''
        with tempfile.TemporaryDirectory() as directory:
            cpp = Path(directory) / "test.cpp"
            exe = Path(directory) / "test"
            cpp.write_text(code)
            built = subprocess.run(
                ["g++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
                 "-I" + str(FW / "main"), str(cpp), "-o", str(exe)],
                capture_output=True, text=True, timeout=40)
            self.assertEqual(built.returncode, 0, built.stderr)
            run = subprocess.run([str(exe)], capture_output=True, text=True,
                                 timeout=20)
            self.assertEqual(run.returncode, 0, run.stderr)

    def test_persisted_checkpoint_contract_without_rf_side_effects(self):
        main = (FW / "main/main.cpp").read_text("utf-8")
        crash = (FW / "main/rflink_crash_report.cpp").read_text("utf-8")
        web = (FW / "main/rflink_web_ota.cpp").read_text("utf-8")
        self.assertIn("boot_phase_begin(phase_history, snapshot.boot_sequence)", crash)
        self.assertIn("boot_phase_set(snapshot.phase_history, snapshot.boot_sequence", crash)
        self.assertIn("kPhaseKey", crash)
        self.assertIn("nvs_commit(handle)", crash)
        self.assertIn("phase_history_saved", crash)
        self.assertIn("boot_phases", crash)
        self.assertNotIn("esp_core_dump_image_get", crash)
        for phase in ("probing_radios", "starting_receivers",
                      "starting_network", "verifying_ota", "app_ready"):
            self.assertIn("BootPhase::" + phase, main)
        self.assertIn("startup_completed", main)
        self.assertIn("phaseByBoot", web)
        self.assertIn("last reached", web)

    def test_builder_only_shows_allowed_stages_without_credentials(self):
        sys.path.insert(0, str(ROOT / "rflink_builder"))
        from diagnostic_proxy import _safe_crash_report
        report = _safe_crash_report({
            "history": [{"boot": 2, "reason": "Task watchdog"}],
            "phase_history_saved": True,
            "boot_phases": [
                {"boot": 1, "stage": "starting_receivers", "startup_complete": False,
                 "password": "PRIVATE-WIFI-PASSWORD"},
                {"boot": 2, "stage": "app_ready", "startup_complete": True},
                {"boot": 3, "stage": "<script>PRIVATE</script>",
                 "startup_complete": True},
                {"boot": False, "stage": "nvs_ready", "startup_complete": True},
                {"boot": 4, "stage": "starting_network", "startup_complete": "yes"},
            ],
        })
        self.assertTrue(report["phase_history_saved"])
        self.assertEqual([x["boot"] for x in report["boot_phases"]], [1, 2, 4])
        self.assertFalse(report["boot_phases"][-1]["startup_complete"])
        self.assertNotIn("PRIVATE", str(report))
        js = (ROOT / "rflink_builder/static/rf_console.js").read_text("utf-8")
        self.assertIn("rf-diag-boot-phase", js)
        self.assertIn("phaseByBoot", js)


if __name__ == "__main__":
    unittest.main()
