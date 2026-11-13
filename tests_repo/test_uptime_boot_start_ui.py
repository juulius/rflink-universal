"""Boot-start timestamp is shown next to uptime without changing radio code."""
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


class UptimeBootStartUi(unittest.TestCase):
    def test_firmware_and_builder_share_clock_presentation(self):
        for folder in ("firmware/main/webui", "rflink_builder/static/rflink_web"):
            js = (ROOT / folder / "app.js").read_text(encoding="utf-8")
            css = (ROOT / folder / "app.css").read_text(encoding="utf-8")
            html = (ROOT / folder / "index.html").read_text(encoding="utf-8")
            self.assertIn("renderUptimeWithBoot(s)", js)
            self.assertIn("function renderUptimeWithBoot(status)", js)
            self.assertIn("document.createTextNode(uptime(status.uptime_s))", js)
            self.assertIn("status.clock_synced===true", js)
            self.assertIn("localTimestamp(status.boot_started_utc_ms)", js)
            self.assertIn("'Start time unknown · awaiting NTP'", js)
            self.assertIn("' · boot #'+status.boot_id", js)
            self.assertIn(".uptime-boot-hint", css)
            self.assertIn('id="info-uptime"', html)
            self.assertIn('id="info-clock"', html)
            # Existing separate clock and log event detail paths remain functional.
            self.assertIn("localTimestamp(s.now_utc_ms)", js)
            self.assertIn("localTimestamp(e.timestamp_utc_ms)", js)
            self.assertIn("uptimeMs(e.uptime_ms)", js)

    def test_backend_boot_clock_fields(self):
        source = (ROOT / "firmware/main/rflink_web_ota.cpp").read_text()
        for key in ('clock_synced', 'boot_id', 'boot_started_utc_ms',
                    'now_utc_ms', 'uptime_s'):
            self.assertIn(key, source)


if __name__ == "__main__":
    unittest.main()
