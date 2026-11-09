"""Native v0.7 browser paths must not regress to ESPHome v0.6.6 endpoints.

This is an offline source contract, not a hardware/UI parity test.
"""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[1]
WEB=(ROOT/'firmware/main/rflink_web_ota.cpp').read_text('utf-8')
PROXY=(ROOT/'rflink_builder/diagnostic_proxy.py').read_text('utf-8')
DEVICE=(ROOT/'rflink_builder/devices.py').read_text('utf-8')


class NativeApiRoutingContract(unittest.TestCase):
    def test_native_status_route_and_browser_are_root_relative(self):
        self.assertIn('status.uri="/api/status"',WEB)
        self.assertIn("fetch('/api/status'",WEB)
        self.assertNotIn('/rflink/api/status',WEB)

    def test_native_status_is_used_by_builder_device_probes(self):
        self.assertIn('"/api/status"',PROXY)
        self.assertIn('"/api/status"',DEVICE)
        self.assertNotIn('/rflink/api/status',PROXY)
        self.assertNotIn('/rflink/api/status',DEVICE)

    def test_native_and_builder_runtime_js_do_not_use_esphome_rflink_api(self):
        for script in (ROOT/'rflink_builder/static').glob('*.js'):
            with self.subTest(script=script.name):
                self.assertNotIn('/rflink/api/',script.read_text('utf-8'))
        self.assertNotIn('/rflink/api/',WEB)

    def test_migration_does_not_claim_old_rest_handlers_work(self):
        self.assertIn('logs.uri="/api/logs"',WEB)
        self.assertIn('crashes.uri="/api/diagnostics/crashes"',WEB)
        self.assertIn('ota.uri="/api/ota"',WEB)
        plan=(ROOT/'docs/NATIVE_UI_066_MIGRATION.md').read_text('utf-8')
        for page in ('Status','Devices','Logs','Add device','Settings'):
            self.assertIn(page,plan)
        self.assertIn('**No device CRUD/control HTTP API**',plan)


if __name__=='__main__':
    unittest.main()
