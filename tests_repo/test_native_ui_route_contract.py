"""Native v0.7 browser paths must not regress to ESPHome v0.6.6 endpoints.

This is an offline source contract, not a hardware/UI parity test.
"""
from pathlib import Path
import re
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

    def test_no_runtime_legacy_api_urls_in_native_ui_or_builder(self):
        # Find literal legacy API addresses in every shipped HTML, JS and
        # Python runtime module. Docs/tests can refer to them intentionally.
        # Only the native HTTP 404 handler may mention the old prefix to
        # diagnose a client still requesting it; it must not serve that API.
        roots=(ROOT/'firmware/main/webui',ROOT/'rflink_builder/static',
               ROOT/'rflink_builder')
        seen=set()
        for root in roots:
            if root.name=='static':
                files=(*root.glob('*.js'),*root.glob('*.html'))
            elif root.name=='webui':
                files=(*root.glob('*.js'),*root.glob('*.html'))
            else:
                files=(*root.glob('*.py'),)
            for file in files:
                if file in seen:
                    continue
                seen.add(file)
                source=file.read_text('utf-8')
                # Catch single/double-quoted URL references and dynamically
                # concatenated prefixes; no search-only exclusions.
                bad=re.search(r"""['"](/rflink/api(?:/|\b))""",source)
                self.assertIsNone(bad, f"{file}: legacy API URL detected")
        self.assertIn('httpd_register_err_handler(server,HTTPD_404_NOT_FOUND',
                      WEB)
        self.assertIn('ESP_LOGW(TAG,"Legacy API 404:',WEB)
        self.assertIn('native UI is / and API is /api/',WEB)
        self.assertIn('console.uri="/api/diagnostics/console"',WEB)
        self.assertIn('status.uri="/api/status"',WEB)
        console_handler=WEB[WEB.index('static esp_err_t console_get'):
                            WEB.index('static esp_err_t crashes_get')]
        self.assertNotIn('key_ok(req)',console_handler)
        self.assertIn('static bool unsafe_console_text',WEB)
        ota_handler=WEB[WEB.index('static esp_err_t ota_post'): ]
        self.assertIn('if(!key_ok(req))',ota_handler)

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
