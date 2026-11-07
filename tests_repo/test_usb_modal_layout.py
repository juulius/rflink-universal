"""UI and security regression checks for RFLink Builder 0.6.14 USB popups.

These assertions verify the bundled browser implementation and policy; the
actual Windows COM-port chooser, flash result and HA Ingress permission policy
still require a physical ESP32-S3 test.
"""
import re
import unittest
from html.parser import HTMLParser
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
STATIC=ROOT/'rflink_builder/static'
VENDOR=STATIC/'vendor/esp-web-tools'

class IdCollector(HTMLParser):
    def __init__(self):
        super().__init__()
        self.tags={}
    def handle_starttag(self,tag,attrs):
        props=dict(attrs)
        if 'id' in props:self.tags[props['id']]=(tag,props)

class BuilderModalTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.html=(STATIC/'index.html').read_text('utf-8')
        cls.css=(STATIC/'app.css').read_text('utf-8')
        cls.browser=(STATIC/'browser_flash.js').read_text('utf-8')
        cls.device=(STATIC/'device_firmware.js').read_text('utf-8')
        cls.server=(ROOT/'rflink_builder/server.py').read_text('utf-8')
        cls.dom=IdCollector()
        cls.dom.feed(cls.html)

    def test_single_scrolling_build_log_with_visible_controls(self):
        self.assertIn('id="firmware-build-dialog"',self.html)
        self.assertIn('id="firmware-build-log"',self.html)
        self.assertIn('id="firmware-build-close"',self.html)
        self.assertIn('.firmware-build-dialog[open]{',self.css)
        self.assertIn('height:min(86dvh,780px)',self.css)
        self.assertIn('.firmware-build-dialog .firmware-build-body{',self.css)
        self.assertIn('overflow:hidden;gap:10px',self.css)
        self.assertIn('min-height:70px;height:auto;max-height:none',self.css)
        self.assertIn('overflow-y:auto;overflow-x:auto;resize:none',self.css)
        self.assertIn('Closing this window does not stop the build.',self.html)

    def test_usb_serial_popup_stays_inside_builder_when_permitted(self):
        self.assertIn('id="browser-usb-dialog"',self.html)
        self.assertIn('id="browser-usb-status"',self.html)
        self.assertIn('id="browser-usb-direct"',self.html)
        self.assertIn('id="browser-usb-external"',self.html)
        self.assertIn('vendor/esp-web-tools/install-button.js',self.html)
        self.assertIn('customElements.whenDefined',self.browser)
        self.assertIn('browserUsbDialog.showModal()',self.browser)
        self.assertIn('browserUsbAllowed()',self.browser)
        self.assertIn("policy.allowsFeature('serial')",self.browser)
        self.assertIn('chipFamily:\'ESP32-S3\'',self.browser)
        self.assertIn('offset:0',self.browser)
        self.assertIn('manifestUrl',self.browser)
        self.assertIn('new_install_prompt_erase:true',self.browser)
        self.assertIn('new_install_improv_wait_time:0',self.browser)

    def test_external_tab_is_user_initiated_permission_fallback(self):
        self.assertIn("browserUsbExternal.addEventListener('click',launchExternalBrowserUsb)",self.browser)
        self.assertIn("const tab=window.open('about:blank','_blank')",self.browser)
        self.assertIn('event.origin!==origin||event.source!==tab',self.browser)
        self.assertIn("msg.type==='esphome-web-flash:ready'",self.browser)
        self.assertIn('https://web.esphome.io/',self.browser)
        self.assertIn('browserUsbMessage(',self.browser)
        first=self.browser.split('async function installBrowserFirmware(d)',1)[1].split(
            'async function launchExternalBrowserUsb',1)[0]
        self.assertNotIn('window.open(',first,'PC USB must open an in-app popup, not a new tab')

    def test_no_typed_safety_code_and_backend_scope_preserved(self):
        self.assertEqual(self.dom.tags['detail-install-confirm'][0],'input')
        self.assertEqual(self.dom.tags['detail-install-confirm'][1].get('type'),'checkbox')
        self.assertNotIn('placeholder="TESTBOARD"',self.html)
        self.assertIn('installConfirm.checked',self.device)
        self.assertIn("confirm:'TESTBOARD'",self.browser)
        self.assertIn("if payload.get('confirm') != 'TESTBOARD':",self.server)
        self.assertIn("'X-RFLink-CSRF':csrf",self.browser)
        self.assertIn('activateDeviceFirmware(d)',self.browser)
        self.assertNotIn('window.location.href',self.browser)

    def test_browser_flasher_is_pinned_locally_without_cdn_scripts(self):
        assets=list(VENDOR.glob('*.js'))
        self.assertGreaterEqual(len(assets),10)
        self.assertLess(sum(path.stat().st_size for path in assets),1000000)
        self.assertTrue((VENDOR/'install-button.js').exists())
        self.assertTrue((VENDOR/'LICENSE').exists())
        self.assertIn('Apache License', (VENDOR/'LICENSE').read_text('utf-8'))
        self.assertIn('script-src \'self\'',self.server)
        self.assertIn('connect-src \'self\' blob:',self.server)
        self.assertIn("path.startswith('/vendor/esp-web-tools/')",self.server)
        self.assertIn('file.is_relative_to(root)',self.server)
        self.assertNotIn('unpkg.com',self.browser)
        self.assertNotIn('cdn.jsdelivr',self.browser)

if __name__=='__main__':
    unittest.main()
