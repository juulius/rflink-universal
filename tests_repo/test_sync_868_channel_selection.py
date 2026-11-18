"""Verify actual CMake selection of 868 RX on main/Experimental vs RC/Stable.

The Home Assistant Builder passes the selected immutable tag through
RFLINK_RELEASE_TAG, while untagged main builds have no such environment value.
This compiles a disposable CMake-only stand-in target (not ESP-IDF firmware).
"""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
MAIN_CMAKE = ROOT / "firmware/main/CMakeLists.txt"

@unittest.skipUnless(shutil.which("cmake"), "CMake required")
class Sync868ChannelSelection(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        # Extract the exact release-mode logic from our actual ESP-IDF
        # component CMake file, not a duplicate/reimplemented Python rule.
        original = MAIN_CMAKE.read_text()
        marker = "# Active 868-MHz transport is selected"
        self.assertIn(marker, original)
        selection = original[original.index(marker):]
        (self.root / "CMakeLists.txt").write_text(
            "cmake_minimum_required(VERSION 3.16)\n"
            "project(rflink_sync_channel CXX)\n"
            "file(WRITE ${CMAKE_BINARY_DIR}/dummy.cpp \"void noop(){}\\n\")\n"
            "add_library(dummy STATIC ${CMAKE_BINARY_DIR}/dummy.cpp)\n"
            "set(COMPONENT_LIB dummy)\n"
            + selection
        )

    def configure(self, tag=None, *, explicit=None, build="out", expected=True):
        env = dict(os.environ)
        env.pop("RFLINK_RELEASE_TAG", None)
        if tag is not None:
            env["RFLINK_RELEASE_TAG"] = tag
        cmd = ["cmake", "-S", str(self.root), "-B", str(self.root/build)]
        if explicit is not None:
            cmd.append("-DRFLINK_ENABLE_SYNC_868_BENCH=" + explicit)
        result = subprocess.run(cmd, capture_output=True, text=True, env=env,
                                timeout=35, check=False)
        if expected:
            self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
            cache = (self.root/build/"CMakeCache.txt").read_text()
            expected_flag = (
                "RFLINK_ENABLE_SYNC_868_BENCH:BOOL=" +
                ("ON" if tag is None or "-exp." in tag else "OFF")
            )
            self.assertIn(expected_flag, cache)
        else:
            self.assertNotEqual(result.returncode, 0, result.stdout)
            self.assertIn("868 RX transport does not match release channel",
                          result.stdout+result.stderr)

    def test_untagged_main_and_experimental_default_to_sync(self):
        self.configure()
        self.configure("firmware/v0.7.0-exp.1", build="exp1")
        self.configure("firmware/v0.7.0-exp.57", build="exp57")

    def test_rc_and_stable_keep_fifo(self):
        self.configure("firmware/v0.7.0-rc.1", build="rc1")
        self.configure("firmware/v0.7.0", build="stable")

    def test_stale_ota_build_cache_cannot_hide_selected_rx_transport(self):
        self.configure("firmware/v0.7.0-exp.5", build="same")
        self.configure("firmware/v0.7.0-rc.2", build="same", expected=False)
        self.configure("firmware/v0.7.0-rc.3", build="fresh")
        self.configure("firmware/v0.7.0-exp.6", build="fresh", expected=False)

    def test_explicit_conflicting_flag_fails_closed(self):
        self.configure("firmware/v0.7.0-exp.5", explicit="OFF",
                       build="badexp", expected=False)
        self.configure("firmware/v0.7.0", explicit="ON",
                       build="badstable", expected=False)

if __name__ == "__main__":
    unittest.main()
