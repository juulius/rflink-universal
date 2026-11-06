"""Regression checks for the approved device-first RFLink Builder UI."""
from html.parser import HTMLParser
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1] / 'rflink_builder' / 'static'

class Collector(HTMLParser):
    def __init__(self):
        super().__init__()
        self.ids = []
        self.elements = []
    def handle_starttag(self, tag, attrs):
        d = dict(attrs)
        if 'id' in d:
            self.ids.append(d['id'])
        self.elements.append((tag, d))

class DeviceFirstUITests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.html = (ROOT/'index.html').read_text(encoding='utf-8')
        cls.js = (ROOT/'devices.js').read_text(encoding='utf-8')
        cls.app = (ROOT/'app.js').read_text(encoding='utf-8')
        cls.css = (ROOT/'app.css').read_text(encoding='utf-8')
        cls.parsed = Collector()
        cls.parsed.feed(cls.html)

    def test_all_dom_ids_unique_and_referenced_elements_present(self):
        ids = self.parsed.ids
        self.assertEqual(len(ids), len(set(ids)), 'IDs must remain unique after moving controls')
        refs = set(re.findall(r"""getElementById\(['"]([^'"]+)['"]\)""",
                              self.js + self.app))
        self.assertFalse(refs - set(ids), 'JS references missing HTML elements: '+str(refs-set(ids)))

    def test_only_devices_are_initially_visible(self):
        active = [attrs.get('data-page-section') for _, attrs in self.parsed.elements
                  if 'view-section' in attrs.get('class','').split()
                  and 'active-view' in attrs.get('class','').split()]
        self.assertEqual(active, ['devices'],
                         'Build and settings must not appear under the dashboard at startup')

    def test_mobile_detail_and_desktop_split_are_both_present(self):
        self.assertIn('class="device-list-pane"', self.html)
        self.assertIn('class="device-detail-pane"', self.html)
        self.assertIn('id="detail-back"', self.html)
        self.assertIn('data-detail-tab="overview"', self.html)
        self.assertIn('data-detail-tab="connection"', self.html)
        self.assertIn('data-detail-tab="firmware"', self.html)
        self.assertIn('data-detail-tab="advanced"', self.html)
        self.assertIn('@media(max-width:879px)', self.css)
        self.assertIn('.device-workspace.device-open .device-list-pane{display:none}', self.css)
        self.assertIn('.device-workspace.device-open .device-detail-pane{display:block}', self.css)

    def test_sensitive_controls_are_per_device(self):
        self.assertIn("deviceRequest('key/save'", self.js)
        self.assertIn("deviceRequest('key/reveal'", self.js)
        self.assertIn("hideDetailPassword()", self.js)
        self.assertIn(":not(#detail-key)", self.app,
                      'Generic eye controls must not duplicate the device-specific eye')
        self.assertIn('compatibleBuild.board_profile_hash===d.board_hash', self.js)
        self.assertIn('compatibleBuild.board_id===d.board_id', self.js)
        self.assertIn("ota.disabled=!ready", self.js)
        self.assertIn("TESTBOARD", self.html)

    def test_old_builder_routes_remain_accessible(self):
        self.assertIn('id="advanced-open-build"', self.html)
        self.assertIn('id="advanced-open-versions"', self.html)
        self.assertIn("navigate('build')", self.js)
        self.assertIn("navigate('settings')", self.js)
        self.assertIn("navigate('versions')", self.js)
        self.assertIn('id="toolchain-console"', self.html)

if __name__ == '__main__':
    unittest.main()
