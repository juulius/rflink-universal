"""Safety regression tests for the ESP-IDF build scope gate."""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from ci_build_scope import needs_native_build, safe_without_native_build


class NativeBuildScopeTests(unittest.TestCase):
    def test_firmware_change_requires_real_build(self):
        self.assertTrue(needs_native_build(["firmware/main/main.cpp"]))
        self.assertTrue(needs_native_build(["firmware/SOURCE_MANIFEST.json"]))

    def test_ci_or_release_logic_requires_real_build(self):
        for path in (
            ".github/workflows/validate_source.yml",
            ".github/workflows/auto_firmware_release.yml",
            ".github/workflows/publish_firmware_release.yml",
            "releases/release_auto.py",
        ):
            with self.subTest(path=path):
                self.assertTrue(needs_native_build([path]))

    def test_native_browser_usb_integration_requires_real_build(self):
        for path in (
            "rflink_builder/default_source.zip",
            "rflink_builder/provisioning.py",
            "rflink_builder/browser_flash.py",
            "rflink_builder/toolchain.py",
        ):
            with self.subTest(path=path):
                self.assertTrue(needs_native_build([path]))

    def test_builder_and_documentation_only_skip_build(self):
        self.assertFalse(needs_native_build([
            "rflink_builder/server.py", "rflink_builder/static/app.js",
            "docs/GETTING_STARTED.md", "README.md",
        ]))

    def test_existing_promotion_only_skips_pr_build(self):
        self.assertFalse(needs_native_build([
            "releases/promotions/to-rc.json", "docs/RELEASE.md"
        ]))

    def test_mixed_changes_fail_closed(self):
        self.assertTrue(needs_native_build([
            "rflink_builder/static/app.js", "firmware/components/radio/rx.cpp"
        ]))

    def test_empty_or_unknown_changes_fail_closed(self):
        self.assertTrue(needs_native_build([]))
        self.assertTrue(needs_native_build(["other/unknown.py"]))
        self.assertTrue(needs_native_build(["../firmware/main/escaped.cpp"]))
        self.assertFalse(safe_without_native_build("firmware/main/radio.cpp"))


if __name__ == "__main__":
    unittest.main()
