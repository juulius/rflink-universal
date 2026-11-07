"""Cleanup regression tests; no ESP-IDF toolchain or Home Assistant required."""
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "rflink_builder"))
from build_cleanup import discard_build_workspace


class BuilderCleanupTests(unittest.TestCase):
    def test_removes_disposable_build_but_keeps_releases_and_toolchain(self):
        with tempfile.TemporaryDirectory() as root:
            data = Path(root)
            work = data / "build_workspace"
            (work / "build" / "esp-idf").mkdir(parents=True)
            (work / "build" / "esp-idf" / "large.o").write_bytes(b"x" * 2048)
            (work / "release").mkdir()
            (work / "release" / "temporary.bin").write_bytes(b"temp")
            saved = data / "releases" / "123456789abc"
            saved.mkdir(parents=True)
            (saved / "firmware.bin").write_bytes(b"keep")
            tool = data / "toolchain" / "esp-idf"
            tool.mkdir(parents=True)
            (tool / "idf.py").write_text("keep", encoding="utf-8")

            self.assertTrue(discard_build_workspace(data))
            self.assertFalse(work.exists())
            self.assertEqual((saved / "firmware.bin").read_bytes(), b"keep")
            self.assertEqual((tool / "idf.py").read_text("utf-8"), "keep")
            self.assertFalse(discard_build_workspace(data))

    def test_never_follows_symlink_or_deletes_unexpected_file(self):
        with tempfile.TemporaryDirectory() as root:
            data = Path(root)
            protected = data / "keep"
            protected.mkdir()
            (protected / "secret.txt").write_text("safe", encoding="utf-8")
            workspace = data / "build_workspace"
            workspace.symlink_to(protected, target_is_directory=True)
            with self.assertRaisesRegex(OSError, "symlinked"):
                discard_build_workspace(data)
            self.assertEqual((protected / "secret.txt").read_text("utf-8"), "safe")
            workspace.unlink()
            workspace.write_text("not a directory", encoding="utf-8")
            with self.assertRaisesRegex(OSError, "not a directory"):
                discard_build_workspace(data)
            self.assertTrue(workspace.is_file())


if __name__ == "__main__":
    unittest.main()
