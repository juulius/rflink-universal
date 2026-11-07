"""Offline regression tests for PR-gated firmware release numbering and promotion."""
import importlib.util
import json
import os
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("rflink_release_auto", ROOT / "releases/release_auto.py")
rel = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = rel
spec.loader.exec_module(rel)
CONFIG = {"version": "0.7.0", "codename": "Radioactive", "automatic_experimental": True}


class ReleaseAutomationTests(unittest.TestCase):
    def test_post_merge_change_list_covers_rebase_and_renames(self):
        files = [
            {"filename": "firmware/main/main.cpp", "status": "modified"},
            {"filename": "releases/promotions/new.json", "status": "renamed",
             "previous_filename": "releases/promotions/old.json"},
        ]
        with tempfile.TemporaryDirectory() as temp:
            file = Path(temp) / "changed.json"
            file.write_text(json.dumps(files))
            with patch.dict(os.environ, {"RFLINK_PR_FILES_JSON": str(file)}):
                rows = rel.changes("HEAD^1", "HEAD")
        self.assertIn(("M", "firmware/main/main.cpp"), rows)
        self.assertIn(("D", "releases/promotions/old.json"), rows)
        self.assertIn(("A", "releases/promotions/new.json"), rows)

    def test_firmware_pr_creates_next_experimental_only_after_merge(self):
        with patch.object(rel, "config", return_value=CONFIG), patch.object(
            rel, "changes", return_value=[("M", "firmware/main/main.cpp")]
        ):
            report = rel.plan("base", "head")
        self.assertEqual(report["action"], "experimental")
        self.assertEqual(report["channel"], "experimental")
        self.assertNotIn("tag", report)

    def test_builder_and_documentation_changes_do_not_publish_firmware(self):
        with patch.object(rel, "config", return_value=CONFIG), patch.object(
            rel, "changes", return_value=[("M", "rflink_builder/server.py"),
                                         ("M", "docs/V0_7_STATUS.md")]
        ):
            self.assertEqual(rel.plan("base", "head")["action"], "skip")

    def test_next_sequence_is_max_existing_not_number_of_merges(self):
        tags = ("sha\trefs/tags/firmware/v0.7.0-exp.1\n"
                "sha\trefs/tags/firmware/v0.7.0-exp.12\n"
                "sha\trefs/tags/firmware/v0.7.0-exp.3\n"
                "sha\trefs/tags/firmware/v0.7.0-rc.1\n")
        with patch.object(rel, "command", return_value=tags):
            self.assertEqual(rel.next_sequence("0.7.0", "exp"), 13)
            self.assertEqual(rel.next_sequence("0.7.0", "rc"), 2)

    def test_experimental_release_pins_merged_commit(self):
        merged = "a" * 40
        with patch.object(rel, "next_sequence", return_value=2), patch.object(
            rel, "command", return_value=""
        ):
            report = rel.resolve({"channel": "experimental", "config": CONFIG}, merged)
        self.assertEqual(report["source_sha"], merged)
        self.assertEqual(report["tag"], "firmware/v0.7.0-exp.2")
        self.assertEqual(report["sequence"], 2)

    def test_post_merge_cli_resolves_symbolic_head_to_commit_sha(self):
        plan = {"action": "experimental", "channel": "experimental", "config": CONFIG}
        with patch.object(sys, "argv", ["release_auto.py", "resolve", "--base", "HEAD^1",
                                        "--head", "HEAD"]), patch.object(
            rel, "plan", return_value=plan
        ), patch.object(rel, "published_for_merge", return_value=None), patch.object(
            rel, "command", return_value="a" * 40 + "\n"
        ) as command, patch.object(
            rel, "resolve", return_value={"tag": "firmware/v0.7.0-exp.1",
                                          "source_sha": "a" * 40}
        ) as resolve:
            self.assertEqual(rel.cli(), 0)
        command.assert_called_once_with("git", "rev-parse", "--verify", "HEAD")
        resolve.assert_called_once_with(plan, "a" * 40)

    def test_publication_rerun_detects_existing_release_from_merge_sha(self):
        sha = "a" * 40
        published = [[{"tag_name": "firmware/v0.7.0-exp.5",
                       "draft": False, "body": "Merge commit: " + sha},
                      {"tag_name": "firmware/v0.7.0-exp.6",
                       "draft": False, "body": "Merge commit: " + "b" * 40}]]
        with patch.dict(os.environ, {"GITHUB_REPOSITORY": "juulius/rflink-universal"}), patch.object(
            rel, "command", return_value=json.dumps(published)
        ):
            self.assertEqual(rel.published_for_merge(sha), "firmware/v0.7.0-exp.5")
            self.assertIsNone(rel.published_for_merge("c" * 40))

    def test_stable_publication_freezes_that_version_family(self):
        with patch.object(rel, "command", return_value="sha\trefs/tags/firmware/v0.7.0\n"):
            with self.assertRaisesRegex(ValueError, "already Stable"):
                rel.resolve({"channel": "experimental", "config": CONFIG}, "a" * 40)

    def test_promotion_pins_prior_source_instead_of_main(self):
        with patch.object(rel, "published_tag", return_value="b" * 40), patch.object(
            rel, "next_sequence", return_value=3
        ), patch.object(rel, "command", return_value=""):
            report = rel.resolve({"channel": "pre-release", "config": CONFIG,
                                  "from_tag": "firmware/v0.7.0-exp.9"}, "a" * 40)
        self.assertEqual(report["tag"], "firmware/v0.7.0-rc.3")
        self.assertEqual(report["source_sha"], "b" * 40)
        self.assertNotEqual(report["source_sha"], report["merge_sha"])

    def test_no_existing_stable_tag_may_be_overwritten(self):
        with patch.object(rel, "published_tag", return_value="b" * 40), patch.object(
            rel, "command", return_value="sha\trefs/tags/firmware/v0.7.0\n"
        ):
            with self.assertRaisesRegex(ValueError, "already exists"):
                rel.resolve({"channel": "stable", "config": CONFIG,
                             "from_tag": "firmware/v0.7.0-rc.2"}, "a" * 40)

    def test_rc_and_stable_promotion_shape_and_evidence(self):
        with tempfile.TemporaryDirectory() as temp:
            base = Path(temp)
            folder = base / "releases/promotions"
            folder.mkdir(parents=True)
            with patch.object(rel, "ROOT", base), patch.object(rel, "config", return_value=CONFIG):
                path = "releases/promotions/promote-rc.json"
                (base / path).write_text(json.dumps({
                    "version": "0.7.0", "codename": "Radioactive", "channel": "pre-release",
                    "promote_from": "firmware/v0.7.0-exp.9",
                    "reason": "Real hardware RX check complete; begin release candidate testing."
                }))
                with patch.object(rel, "changes", return_value=[("A", path)]):
                    report = rel.plan("a", "b")
                self.assertEqual(report["channel"], "pre-release")
                stable = "releases/promotions/promote-stable.json"
                evidence = {k: "https://github.com/juulius/rflink-universal/issues/12"
                            for k in rel.EVIDENCE}
                data = {"version": "0.7.0", "codename": "Radioactive",
                        "channel": "stable", "promote_from": "firmware/v0.7.0-rc.2",
                        "reason": "All physical tests passed, documented and human approved.",
                        "stable_approval": "REAL_BOARD_TESTS_COMPLETE",
                        "hardware_evidence": evidence}
                (base / stable).write_text(json.dumps(data))
                with patch.object(rel, "changes", return_value=[("A", stable)]):
                    self.assertEqual(rel.plan("a", "b")["channel"], "stable")
                    data.pop("hardware_evidence")
                    (base / stable).write_text(json.dumps(data))
                    with self.assertRaisesRegex(ValueError, "evidence"):
                        rel.plan("a", "b")
                with patch.object(rel, "changes", return_value=[("A", path),
                                                                ("M", "firmware/main/main.cpp")]):
                    with self.assertRaisesRegex(ValueError, "must not modify firmware"):
                        rel.plan("a", "b")
                with patch.object(rel, "changes", return_value=[("M", path)]):
                    with self.assertRaisesRegex(ValueError, "immutable"):
                        rel.plan("a", "b")

    def test_source_pipeline_and_post_merge_publication_separate(self):
        checks = (ROOT / ".github/workflows/validate_source.yml").read_text()
        auto = (ROOT / ".github/workflows/auto_firmware_release.yml").read_text()
        self.assertIn("releases/release_auto.py pr-check", checks)
        self.assertIn("workflow_run:", auto)
        self.assertIn("workflow_run.conclusion == 'success'", auto)
        self.assertIn("workflow_run.event == 'push'", auto)
        self.assertIn("git fetch --tags origin", auto)
        self.assertIn("python3 tools/source_manifest.py --verify", auto)
        self.assertIn("python3 tools/package_release.py", auto)
        self.assertIn('if [ "$CHANNEL" = stable ]', auto)
        self.assertIn('source_sha', auto)
        self.assertIn("gh release create", auto)


if __name__ == "__main__":
    unittest.main()
