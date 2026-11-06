"""Offline regressions for lightweight Home Assistant Builder packaging."""
from pathlib import Path
import struct
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
BUILDER = ROOT / "rflink_builder"

import sys
sys.path.insert(0, str(BUILDER))
from toolchain import IDF_VERSION, TARGET, ToolchainManager


class BuilderPackagingTests(unittest.TestCase):
    def test_home_assistant_png_dimensions(self):
        for filename, size in (("icon.png", (128, 128)), ("logo.png", (250, 100))):
            with self.subTest(filename=filename):
                raw = (BUILDER / filename).read_bytes()
                self.assertEqual(raw[:8], b"\x89PNG\r\n\x1a\n")
                self.assertEqual(struct.unpack(">II", raw[16:24]), size)
        self.assertEqual((BUILDER / "static" / "icon.png").read_bytes(),
                         (BUILDER / "icon.png").read_bytes())

    def test_small_image_no_prebuilt_esp_idf(self):
        docker = (BUILDER / "Dockerfile").read_text("utf-8")
        self.assertIn("FROM python:3.12-slim-bookworm", docker)
        self.assertNotIn("FROM espressif/idf", docker)
        self.assertIn("COPY *.py", docker)
        self.assertIn("COPY static/", docker)
        self.assertIn("toolchain/**", (BUILDER / "config.yaml").read_text("utf-8"))

    def test_persistent_toolchain_marker_and_recovery(self):
        with tempfile.TemporaryDirectory() as root:
            manager = ToolchainManager(Path(root))
            self.assertFalse(manager.is_ready())
            self.assertEqual(manager.snapshot()["status"], "idle")
            manager.idf.joinpath("tools").mkdir(parents=True)
            manager.idf.joinpath("tools/idf.py").touch()
            manager.idf.joinpath("export.sh").touch()
            manager.marker.write_text("v5.5.5\n", encoding="utf-8")
            self.assertFalse(manager.is_ready(), "Wrong IDF version must not pass")
            manager.marker.write_text(IDF_VERSION + "\n", encoding="utf-8")
            self.assertTrue(manager.is_ready())
            self.assertTrue(ToolchainManager(Path(root)).snapshot()["ready"])

    def test_setup_log_is_read_incrementally(self):
        with tempfile.TemporaryDirectory() as root:
            manager = ToolchainManager(Path(root))
            self.assertEqual(manager.read_log()["text"], "")
            manager.log_path.write_bytes(b"line one\nline two\n")
            first = manager.read_log(0)
            self.assertEqual(first["text"], "line one\nline two\n")
            self.assertEqual(first["next_offset"], first["size"])
            self.assertEqual(manager.read_log(first["next_offset"])["text"], "")


if __name__ == "__main__":
    unittest.main()
