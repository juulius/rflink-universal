"""Verify that the Builder's bundled first-install source matches the repository."""
from __future__ import annotations

import unittest
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "firmware"
BUNDLE = ROOT / "rflink_builder" / "default_source.zip"
SKIP_DIRS = {"build", "release", "__pycache__", ".git", ".pytest_cache",
             "managed_components", ".idf_component_manager", "portable_sdk"}
SKIP_ROOT_FILES = {"sdkconfig", "sdkconfig.old", "dependencies.lock"}


def expected_source_files() -> dict[str, bytes]:
    files = {}
    for path in SOURCE.rglob("*"):
        if not path.is_file():
            continue
        rel = path.relative_to(SOURCE)
        if any(part in SKIP_DIRS for part in rel.parts):
            continue
        if len(rel.parts) == 1 and rel.name in SKIP_ROOT_FILES:
            continue
        files["firmware/" + rel.as_posix()] = path.read_bytes()
    return files


class BuilderBundleTests(unittest.TestCase):
    def test_embedded_source_matches_repository(self):
        if not SOURCE.is_dir() or not BUNDLE.is_file():
            self.skipTest("Firmware or Builder source not published yet")
        expected = expected_source_files()
        self.assertTrue(expected, "No native firmware source found")
        with zipfile.ZipFile(BUNDLE) as zipped:
            self.assertIsNone(zipped.testzip(), "Corrupt Builder source ZIP")
            entries = [entry for entry in zipped.infolist() if not entry.is_dir()]
            names = [entry.filename for entry in entries]
            self.assertEqual(len(names), len(set(names)), "Duplicate archive paths")
            self.assertEqual(set(names), set(expected),
                             "Builder default_source.zip is out of sync; regenerate it from firmware/")
            for name in sorted(expected):
                self.assertEqual(zipped.read(name), expected[name],
                                 f"Stale bundled source file: {name}")


if __name__ == "__main__":
    unittest.main()
