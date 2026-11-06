"""Guard rails for staged Home Assistant Builder container publication.

A prebuilt image must be pushed, made public, and tested before adding
the 'image' key that instructs Supervisor to pull it.
"""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
WORKFLOW = ROOT / ".github/workflows/prepare_builder_image.yml"
CONFIG = ROOT / "rflink_builder/config.yaml"


class BuilderImagePipelineTests(unittest.TestCase):
    def test_app_still_installs_locally_until_image_is_published(self):
        text = CONFIG.read_text(encoding="utf-8")
        self.assertNotRegex(text, r"(?m)^image\s*:")
        self.assertRegex(text, r"(?m)^version:\s*0\.6\.7\s*$")

    def test_manual_publication_has_explicit_approval_gate(self):
        source = WORKFLOW.read_text(encoding="utf-8")
        self.assertIn("workflow_dispatch:", source)
        self.assertIn("default: false", source)
        self.assertIn("github.event_name == 'workflow_dispatch' && inputs.publish", source)
        self.assertIn("if: github.event_name == 'workflow_dispatch' && inputs.publish", source)
        self.assertIn("pull_request:", source)

    def test_both_supported_architectures_and_release_tag(self):
        source = WORKFLOW.read_text(encoding="utf-8")
        config = CONFIG.read_text(encoding="utf-8")
        self.assertIn('architectures: \'["amd64","aarch64"]\'', source)
        self.assertIn("prepare-multi-arch-matrix@2026.09.0", source)
        self.assertIn("build-image@2026.09.0", source)
        self.assertIn("publish-multi-arch-manifest@2026.09.0", source)
        self.assertIn("image-name: rflink-builder", source)
        self.assertIn("registry-prefix: ghcr.io/juulius", source)
        self.assertIn("${{ needs.prepare.outputs.version }}", source)
        for arch in ("amd64", "aarch64"):
            self.assertRegex(config, rf"(?m)^- {arch}$")

    def test_guide_requires_public_image_before_switching(self):
        guide = (ROOT/"rflink_builder/DOCS.md").read_text(encoding="utf-8")
        self.assertIn("Make the new GitHub Container Registry (GHCR) package **public**", guide)
        self.assertIn("Only after verification, bump the Builder version", guide)


if __name__ == "__main__":
    unittest.main()
