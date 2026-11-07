"""Contract and native ring-buffer regression tests for Stage 23 crash diagnostics."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
FW = ROOT / "firmware"


class CrashDiagnosticsTests(unittest.TestCase):
    def test_crash_capture_enabled_and_separate_flash_partition(self):
        config = (FW / "sdkconfig.defaults").read_text("utf-8")
        for flag in ("CONFIG_ESP_COREDUMP_ENABLE_TO_FLASH=y",
                     "CONFIG_ESP_COREDUMP_DATA_FORMAT_ELF=y",
                     "CONFIG_ESP_COREDUMP_CHECKSUM_SHA256=y",
                     "CONFIG_ESP_COREDUMP_CHECK_BOOT=y"):
            self.assertIn(flag, config)
        partitions = (FW / "partitions.csv").read_text("utf-8")
        self.assertIn("coredump,data,coredump,0xA20000,0x20000", partitions)

    def test_native_crash_path_and_sensitive_data_guard(self):
        main = (FW / "main/main.cpp").read_text("utf-8")
        web = (FW / "main/rflink_web_ota.cpp").read_text("utf-8")
        crash = (FW / "main/rflink_crash_report.cpp").read_text("utf-8")
        cmake = (FW / "main/CMakeLists.txt").read_text("utf-8")
        self.assertIn("crash_report_record_boot();", main)
        self.assertIn("esp_core_dump_image_check()", crash)
        self.assertIn("esp_core_dump_get_summary", crash)
        self.assertIn("nvs_set_blob", crash)
        self.assertIn("/api/diagnostics/crashes", web)
        self.assertNotIn("esp_core_dump_image_get", web, "Do not expose task stacks without auth")
        self.assertIn("rflink_crash_report.cpp", cmake)
        self.assertIn("espcoredump", cmake)

    def test_bounded_boot_ring_preserves_order_and_recovers_corruption(self):
        source = r'''
#include "rflink_boot_history.hpp"
#include <cassert>
using namespace rflink;
int main() {
  BootHistory h{};
  assert(boot_history_valid(h));
  assert(h.count == 0);
  for (uint32_t i = 1; i <= 20; ++i) {
    BootEntry e{};
    e.reset_reason = i;
    e.crash_pc = 0x40000000 + i;
    e.core_dump = i % 2;
    boot_history_append(h, e);
  }
  assert(h.count == kBootHistoryCapacity);
  assert(boot_history_recent(h, 0).sequence == 20);
  assert(boot_history_recent(h, 0).reset_reason == 20);
  assert(boot_history_recent(h, 11).sequence == 9);
  assert(boot_history_recent(h, 12).sequence == 0);
  h.magic = 1234;
  boot_history_append(h, BootEntry{});
  assert(boot_history_valid(h));
  assert(h.count == 1);
  assert(boot_history_recent(h, 0).sequence == 1);
}
'''
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)
            (path / "test.cpp").write_text(source)
            result = subprocess.run(
                ["g++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
                 "-I" + str(FW / "main"),
                 str(path / "test.cpp"), "-o", str(path / "check")],
                capture_output=True, text=True, timeout=40)
            self.assertEqual(result.returncode, 0, result.stderr)
            check = subprocess.run([str(path / "check")], capture_output=True,
                                   text=True, timeout=20)
            self.assertEqual(check.returncode, 0, check.stderr)


if __name__ == "__main__":
    unittest.main()
