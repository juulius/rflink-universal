"""Builder v0.6.16 regression: exact release → source → build → flash identity.

All tests run offline. A published release with the right board hash/version
but the wrong source SHA must never reach USB, browser USB, or OTA.
"""
import sys
import unittest
from pathlib import Path
from html.parser import HTMLParser

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "rflink_builder"))
from firmware_identity import build_identity, require_selected_build

SHA1 = "a" * 40
SHA2 = "b" * 40
IMAGE = "c" * 64
TAG = "firmware/v0.7.0-exp.2"
DEVICE = {"tag": TAG, "sha": SHA1, "release_id": 123,
          "repo": "juulius/rflink-universal"}


def image(tag=TAG, sha=SHA1):
    return {"version": "0.7.0-exp.2-n16r8-b5e871b06",
            "release_tag": tag, "source_commit_sha": sha,
            "source_fingerprint": "0123456789ab",
            "board_profile_hash": "5e871b06",
            "files": {"app_ota": {"filename": "rflink.bin", "sha256": IMAGE}}}


class ProvenanceTests(unittest.TestCase):
    def test_published_binary_exposes_all_non_secret_identifiers(self):
        actual = require_selected_build(
            image(), DEVICE, {"release_tag": TAG, "source_sha": SHA1})
        self.assertEqual(actual["release_tag"], TAG)
        self.assertEqual(actual["source_sha"], SHA1)
        self.assertEqual(actual["version"], "0.7.0-exp.2-n16r8-b5e871b06")
        self.assertEqual(actual["image_sha256"], IMAGE)
        self.assertEqual(actual["board_profile_hash"], "5e871b06")
        self.assertEqual(actual["source_kind"], "github_release")

    def test_refuses_older_experimental_release(self):
        with self.assertRaisesRegex(RuntimeError, "release changed"):
            require_selected_build(image(tag="firmware/v0.7.0-exp.1"), DEVICE)

    def test_refuses_missing_or_wrong_source_commit(self):
        for wrong in (None, SHA2, "", "a" * 12):
            with self.subTest(wrong=wrong), self.assertRaisesRegex(RuntimeError, "source commit"):
                require_selected_build(image(sha=wrong), DEVICE)

    def test_refuses_inconsistent_saved_build_metadata(self):
        with self.assertRaisesRegex(RuntimeError, "Stored build source"):
            require_selected_build(
                image(), DEVICE, {"release_tag": TAG, "source_sha": SHA2})
        with self.assertRaisesRegex(RuntimeError, "different releases"):
            require_selected_build(
                image(), DEVICE, {"release_tag": "firmware/v0.7.0-exp.1",
                                  "source_sha": SHA1})

    def test_refuses_wrong_version_or_missing_binary_fingerprint(self):
        broken = image()
        broken["version"] = "0.7.0-exp.1-n16r8-b5e871b06"
        with self.assertRaisesRegex(RuntimeError, "version or build identity"):
            require_selected_build(broken, DEVICE)
        broken = image()
        broken["files"]["app_ota"]["sha256"] = ""
        with self.assertRaisesRegex(RuntimeError, "version or build identity"):
            require_selected_build(broken, DEVICE)

    def test_local_development_is_distinct_and_remains_usable(self):
        local = image(tag=None, sha=None)
        local["version"] = "0.7.0-n16r8-s20-b5e871b06"
        self.assertEqual(require_selected_build(local, "current")["source_kind"],
                         "local_development")
        with self.assertRaisesRegex(RuntimeError, "release changed"):
            require_selected_build(local, DEVICE)
        with self.assertRaisesRegex(RuntimeError, "release changed"):
            require_selected_build(image(), "current")

    def test_binary_sha_is_not_mistaken_for_git_commit(self):
        public = build_identity(image())
        self.assertEqual(public["image_sha256"], IMAGE)
        self.assertNotEqual(public["source_sha"], public["image_sha256"])


class BuilderWiringTests(unittest.TestCase):
    def test_all_install_methods_use_one_verification_policy(self):
        server = (ROOT / "rflink_builder/server.py").read_text("utf-8")
        self.assertIn("built['source_commit_sha'] = pinned['sha']", server)
        self.assertIn("require_selected_build(built, pinned)", server)
        self.assertIn("require_selected_build(manifest, config.get('firmware_choice', 'current'))", server)
        self.assertIn("require_selected_build(manifest, chosen, meta)", server)
        self.assertIn("require_selected_build(manifest, selected)", server)
        self.assertIn("require_selected_build(manifest, selection)", server)
        self.assertIn("require_selected_build(manifest,config.get('firmware_choice','current'))", server)

    def test_firmware_ui_uses_selected_source_and_compiled_binary(self):
        html = (ROOT / "rflink_builder/static/index.html").read_text("utf-8")
        js = (ROOT / "rflink_builder/static/device_firmware.js").read_text("utf-8")
        for id in ("detail-install-identity","detail-proof-release","detail-proof-source",
                   "detail-proof-version","detail-proof-binary","detail-proof-board",
                   "firmware-build-origin"):
            self.assertIn('id="' + id + '"', html)
        self.assertIn("b.source_sha===selected.sha", js)
        self.assertIn("build.image_sha256", js)
        self.assertIn("build.source_fingerprint", js)
        self.assertIn("renderFirmwareProof(built,d)", js)
        self.assertIn("live===build.version", js)
        self.assertLess(html.index('id="detail-install-identity"'),
                        html.index('class="device-install-methods"'))
        self.assertIn("Compiler <span>&amp; recovery</span>", html)
        self.assertIn("Global defaults", html)
        self.assertIn("Firmware tab", html)
        self.assertIn("v0.6.23", html)


if __name__ == "__main__":
    unittest.main()
