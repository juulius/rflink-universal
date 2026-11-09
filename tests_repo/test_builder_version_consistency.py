"""Keep Home Assistant add-on, image label and visible Builder version in sync."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1] / "rflink_builder"


class BuilderVersionConsistencyTests(unittest.TestCase):
    def test_release_versions_are_identical(self):
        config = (ROOT / "config.yaml").read_text(encoding="utf-8")
        docker = (ROOT / "Dockerfile").read_text(encoding="utf-8")
        page = (ROOT / "static/index.html").read_text(encoding="utf-8")
        notes = (ROOT / "CHANGELOG.md").read_text(encoding="utf-8")

        def version(pattern, data):
            match = re.search(pattern, data, re.MULTILINE)
            self.assertIsNotNone(match, pattern)
            return match.group(1)

        addon = version(r"^version:\s*(\d+\.\d+\.\d+)\s*$", config)
        image = version(r"^ARG BUILD_VERSION=(\d+\.\d+\.\d+)\s*$", docker)
        ui = version(r'<span class="version">v(\d+\.\d+\.\d+)</span>', page)
        self.assertEqual(addon, image)
        self.assertEqual(addon, ui)
        self.assertIn(f"## v{addon} ", notes)


if __name__ == "__main__":
    unittest.main()
