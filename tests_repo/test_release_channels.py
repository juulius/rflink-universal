"""Offline release-channel behavior and immutable-source selection tests."""
import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "rflink_builder"))

from release_history import ReleaseHistory, normalise_release, release_identity


def release(tag, ident, *, prerelease=False, date="2026-11-07T10:00:00Z"):
    return {"id": ident, "tag_name": tag, "name": tag, "published_at": date,
            "prerelease": prerelease, "draft": False, "assets": [], "body": ""}


class ReleaseChannelTests(unittest.TestCase):
    def test_display_is_semantic_and_never_a_commit_hash(self):
        self.assertEqual(
            release_identity("firmware/v0.7.0-exp.14"),
            {"version": "v0.7.0", "channel": "experimental",
             "build_number": 14, "display": "v0.7.0 · Experimental · build 14"})
        self.assertEqual(release_identity("firmware/v0.7.0-rc.2")["display"],
                         "v0.7.0 · Pre-release · RC 2")
        self.assertEqual(release_identity("firmware/v0.7.0")["display"],
                         "v0.7.0 · Stable")
        self.assertEqual(release_identity("firmware/v0.7.0", True)["channel"], "pre-release")
        self.assertIsNone(release_identity("builder/v0.6.11"))

    def test_preview_requires_github_prerelease_flag(self):
        self.assertIsNone(normalise_release(release("firmware/v0.7.0-exp.3", 1)))
        self.assertIsNotNone(normalise_release(
            release("firmware/v0.7.0-exp.3", 1, prerelease=True)))
        self.assertIsNone(normalise_release(release("firmware/v0.7.0-rc.1", 2)))
        self.assertIsNotNone(normalise_release(release("firmware/v0.7.0", 3)))

    def test_latest_channel_chooses_version_then_build_not_merge_order(self):
        rows = [
            release("firmware/v0.7.0-exp.12", 20, prerelease=True),
            release("firmware/v0.7.0-exp.2", 12, prerelease=True,
                    date="2026-11-08T10:00:00Z"),
            release("firmware/v0.7.0-rc.2", 30, prerelease=True),
            release("firmware/v0.7.0-rc.1", 28, prerelease=True),
            release("firmware/v0.6.9", 40),
            release("firmware/v0.6.10", 41),
            release("builder/v0.6.11", 42),
        ]
        def fake(url, limit):
            if "releases?per_page" in url:
                return json.dumps(rows).encode()
            raise AssertionError(url)
        history = ReleaseHistory(fetch=fake)
        current = history.channels("some/firmware")["channels"]
        self.assertEqual(current["experimental"]["tag"], "firmware/v0.7.0-exp.12")
        self.assertEqual(current["pre-release"]["tag"], "firmware/v0.7.0-rc.2")
        self.assertEqual(current["stable"]["tag"], "firmware/v0.6.10")
        self.assertTrue(all("display" in x for x in current.values()))

    def test_experimental_pagination_does_not_hide_stable_history(self):
        previews = [release(f"firmware/v0.7.0-exp.{n}", n,
                            prerelease=True) for n in range(30, 0, -1)]
        stable = [release(f"firmware/v0.6.{n}", n + 100) for n in range(9, 2, -1)]
        def fake(url, limit):
            if "page=1" in url:
                return json.dumps(previews).encode()
            if "page=2" in url:
                return json.dumps(stable).encode()
            raise AssertionError(url)
        history = ReleaseHistory(fetch=fake)
        result = history.recent("some/firmware")
        self.assertEqual(result["latest"]["tag"], "firmware/v0.6.9")
        self.assertEqual(len(result["recent"]), 5)

    def test_without_published_versions_no_fake_stable(self):
        history = ReleaseHistory(fetch=lambda *_: b"[]")
        self.assertEqual(history.channels("some/firmware")["channels"],
                         {"stable": None, "pre-release": None, "experimental": None})

    def test_selected_channel_resolves_immutable_sha(self):
        tag = "firmware/v0.7.0-exp.14"
        def fake(url, limit):
            if "/releases/tags/" in url:
                return json.dumps(release(tag, 14, prerelease=True)).encode()
            if "/commits/" in url:
                return json.dumps({"sha": "c" * 40}).encode()
            raise AssertionError(url)
        history = ReleaseHistory(fetch=fake)
        pinned = history.resolve("some/firmware", tag, 14)
        self.assertEqual(pinned["sha"], "c" * 40)
        self.assertEqual(pinned["channel"], "experimental")
        self.assertEqual(pinned["build_number"], 14)

    def test_builder_ui_offers_test_opt_in_and_channels(self):
        html = (ROOT / "rflink_builder/static/index.html").read_text()
        wizard = (ROOT / "rflink_builder/static/wizard.js").read_text()
        app = (ROOT / "rflink_builder/static/app.js").read_text()
        server = (ROOT / "rflink_builder/server.py").read_text()
        self.assertIn('id="release-channel"', html)
        self.assertIn('id="wizard-include-test"', html)
        self.assertIn("if(wizardIncludeTest.checked)", wizard)
        self.assertIn("channels.stable.tag", wizard)
        self.assertIn("api/releases?scope=channels", app)
        self.assertIn("if scope=='channels':", server)


if __name__ == "__main__":
    unittest.main()
