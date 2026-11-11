"""Host-only checks for one-hour background release cadence and OTA run identity.

No real GitHub request or ESP32 network traffic is made.
"""
from pathlib import Path
from unittest.mock import Mock
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "rflink_builder"))
from release_history import ReleaseAutoChecker


class ReleaseCadenceTests(unittest.TestCase):
    def test_checks_once_per_hour_even_when_browser_is_closed(self):
        clock = [60.0]
        history = Mock()
        checker = ReleaseAutoChecker(history, lambda: "juulius/rflink-universal",
                                     clock=lambda: clock[0])
        self.assertTrue(checker.check_if_due())
        history.page.assert_called_once_with("juulius/rflink-universal", 1)
        clock[0] = 3659.9
        self.assertFalse(checker.check_if_due())
        clock[0] = 3660.0
        self.assertTrue(checker.check_if_due())
        self.assertEqual(history.page.call_count, 2)

    def test_failed_checks_are_not_retried_every_minute(self):
        clock = [100.0]
        history = Mock()
        history.page.side_effect = OSError("GitHub offline")
        checker = ReleaseAutoChecker(history, lambda: "juulius/rflink-universal",
                                     clock=lambda: clock[0])
        with self.assertRaises(OSError):
            checker.check_if_due()
        clock[0] = 110
        self.assertFalse(checker.check_if_due())
        clock[0] = 3700
        with self.assertRaises(OSError):
            checker.check_if_due()
        self.assertEqual(history.page.call_count, 2)

    def test_unconfigured_repo_skips_network_without_short_loop(self):
        clock = [1]
        history = Mock()
        checker = ReleaseAutoChecker(history, lambda: "", clock=lambda: clock[0])
        self.assertFalse(checker.check_if_due())
        history.page.assert_not_called()
        clock[0] += 60
        self.assertFalse(checker.check_if_due())
        clock[0] += 3600
        self.assertFalse(checker.check_if_due())
        history.page.assert_not_called()

    def test_frontend_has_fifteen_minute_interval_and_no_extra_daemon(self):
        chooser = (ROOT / "rflink_builder/static/firmware_catalog.js").read_text("utf-8")
        server = (ROOT / "rflink_builder/server.py").read_text("utf-8")
        self.assertIn("FIRMWARE_POLL_MS=15*60*1000", chooser)
        self.assertIn("release_history, lambda: github_configuration().get('repo', '')", server)
        self.assertIn("auto_releases.check_if_due()", server)
        self.assertIn("device_book.refresh_known()", server)
        self.assertIn("INTERVAL_SECONDS = 60 * 60",
                      (ROOT / "rflink_builder/release_history.py").read_text("utf-8"))


class DeploymentProgressGuardTests(unittest.TestCase):
    def test_deployment_id_is_generated_for_every_new_start(self):
        server = (ROOT / "rflink_builder/server.py").read_text("utf-8")
        panel = (ROOT / "rflink_builder/static/device_firmware.js").read_text("utf-8")
        progress = (ROOT / "rflink_builder/static/rf_flash.js").read_text("utf-8")
        self.assertIn("self.deployment_id=uuid.uuid4().hex[:16]", server)
        self.assertIn("'deployment_id':self.deployment_id", server)
        self.assertIn("'deployment_id':deployment.snapshot()['deployment_id']", server)
        self.assertIn("dep.deployment_id!==expectedId", panel)
        self.assertIn("output.deployment_id!==expectedId", panel)
        self.assertIn("trackedDeploymentId=response.deployment.deployment_id", panel)
        self.assertIn("deployment?.deployment_id!==rfFlashDeploymentId", progress)
        self.assertIn("rfFlashHostStart(method,d,trackedDeploymentId)", panel)

    def test_upload_is_not_displayed_as_fully_verified(self):
        progress = (ROOT / "rflink_builder/static/rf_flash.js").read_text("utf-8")
        self.assertIn("Math.floor(percent*0.9)", progress)
        self.assertIn("stagePercent={awaiting_ack:92,accepted:94,reconnecting:96,validating:98}",progress)
        self.assertIn("state==='success' && ota && deployment.verified===true",progress)
        self.assertIn("rfFlashSetProgress(100,'Verified · new firmware running · health VALID')",progress)


if __name__ == "__main__":
    unittest.main()
