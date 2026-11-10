"""Retired Advanced page, native OTA and memory metrics contracts.

These are source checks, not ESP32 hardware/OTA validation.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
WEB = (ROOT/'firmware/main/rflink_web_ota.cpp').read_text('utf-8')
PAGE = (ROOT/'firmware/main/webui/index.html').read_text('utf-8')
SCRIPT = (ROOT/'firmware/main/webui/app.js').read_text('utf-8')


class NativeAdvancedRetirementTests(unittest.TestCase):
    def test_old_advanced_and_rflink_bookmarks_redirect(self):
        self.assertIn('advanced.handler=native_redirect_get', WEB)
        self.assertIn('legacy.uri="/rflink"', WEB)
        self.assertIn('legacy_slash.uri="/rflink/"', WEB)
        self.assertIn('httpd_resp_set_hdr(req,"Location","/")', WEB)
        self.assertNotIn('href="/advanced"', PAGE)
        self.assertNotIn('static esp_err_t index_get(', WEB)

    def test_initial_wifi_setup_survives(self):
        self.assertIn('if(provisioning)return provisioning_index_get(req)', WEB)
        self.assertIn('action="/setup"', WEB)
        for field in ('ssid', 'pass', 'admin'):
            self.assertIn(f'name="{field}"', WEB)

    def test_ota_api_remains_key_protected_and_checks_inactive_slot(self):
        handler = WEB[WEB.index('static esp_err_t ota_post'):WEB.index('static void event_handler')]
        self.assertIn('if(!key_ok(req))', handler)
        self.assertIn('if(!ota_layout.plan.safe || !ota_layout.target)', handler)
        self.assertIn('esp_ota_set_boot_partition(next)', handler)
        self.assertIn("request.open('POST','/api/ota')", SCRIPT)
        self.assertIn("X-RFLink-Update-Key", SCRIPT)
        self.assertIn('ota_update_safe', SCRIPT)
        self.assertNotIn('localStorage', SCRIPT)
        self.assertNotIn('sessionStorage', SCRIPT)

    def test_memory_fields_are_actual_esp_idf_metrics(self):
        self.assertIn('#include "esp_heap_caps.h"', WEB)
        for field in ('internal_total_bytes', 'internal_free_bytes',
                      'internal_min_free_bytes', 'internal_largest_block_bytes',
                      'psram_total_bytes', 'psram_free_bytes',
                      'psram_min_free_bytes', 'psram_largest_block_bytes'):
            with self.subTest(field=field):
                self.assertIn(field, WEB)
        self.assertIn('MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT', WEB)
        self.assertIn('MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT', WEB)
        self.assertIn("const base='memory-'+kind+'-'", SCRIPT)

    def test_duration_format_and_raw_milliseconds_are_retained(self):
        self.assertIn("String(n).padStart(2,'0')", SCRIPT)
        self.assertIn("join('.')", SCRIPT)
        self.assertIn('uptime(s.uptime_s)', SCRIPT)
        self.assertIn('uptimeMs(e.uptime_ms)', SCRIPT)
        self.assertIn('HH:MM:SS,ms', PAGE)
        self.assertIn('uptime_ms', WEB)
        self.assertNotIn(".toFixed(3)+'s'", SCRIPT)


if __name__=='__main__':
    unittest.main()
