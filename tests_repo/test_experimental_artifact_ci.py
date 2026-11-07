"""Published Experimental builds expose a short-lived test artifact, never production OTA."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class ExperimentalArtifactContract(unittest.TestCase):
    def test_release_workflow_is_manual_and_test_artifact_is_explicit(self):
        workflow = (ROOT / '.github/workflows/publish_firmware_release.yml').read_text()
        self.assertIn('workflow_dispatch:', workflow)
        self.assertIn("if: inputs.channel == 'experimental'", workflow)
        self.assertIn('uses: actions/upload-artifact@v4', workflow)
        self.assertIn('RFLink-Experimental-TESTBOARD-ONLY', workflow)
        self.assertIn('path: firmware/release/', workflow)
        self.assertIn('retention-days: 14', workflow)
        self.assertIn('if-no-files-found: error', workflow)
        self.assertIn('python3 tools/package_release.py', workflow)
        self.assertIn('report["test_only"] is True', workflow)
        self.assertIn('needs: [prepare, tests, compile]', workflow)

    def test_release_artifact_cannot_include_private_per_device_provisioning(self):
        workflow = (ROOT / '.github/workflows/publish_firmware_release.yml').read_text()
        self.assertNotIn('RFLink_device_provisioning_NVS.bin', workflow)
        guide = (ROOT / 'docs/EXPERIMENTAL_BENCH_V07.md').read_text()
        self.assertIn('no device-specific Wi-Fi configuration', guide)
        self.assertIn('Home Assistant RFLink Builder v0.6.12+', guide)


if __name__ == '__main__':
    unittest.main()
