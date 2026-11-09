"""Native RFLink 0.7 UI shell: offline source/route/markup contracts.

These host checks prove that the UI files and route wiring are present and
use valid native API names; they do not prove a running ESP32 or radio behavior.
"""
from html.parser import HTMLParser
from pathlib import Path
import re
import unittest

ROOT=Path(__file__).resolve().parents[1]
WEB=ROOT/'firmware/main/webui'
MAIN=(ROOT/'firmware/main/rflink_web_ota.cpp').read_text('utf-8')
CMAKE=(ROOT/'firmware/main/CMakeLists.txt').read_text('utf-8')
PAGE=(WEB/'index.html').read_text('utf-8')
SCRIPT=(WEB/'app.js').read_text('utf-8')
CSS=(WEB/'app.css').read_text('utf-8')


class Nodes(HTMLParser):
    def __init__(self):
        super().__init__()
        self.ids=set()
        self.pages=set()
        self.links=[]
        self.scripts=[]
    def handle_starttag(self, tag, attrs):
        a=dict(attrs)
        if 'id' in a:
            assert a['id'] not in self.ids, 'duplicate id: '+a['id']
            self.ids.add(a['id'])
        if a.get('data-view'):self.pages.add(a['data-view'])
        if tag=='link':self.links.append(a.get('href'))
        if tag=='script':self.scripts.append(a.get('src'))


class NativeUIShellTests(unittest.TestCase):
    def setUp(self):
        self.nodes=Nodes()
        self.nodes.feed(PAGE)

    def test_full_navigation_and_real_native_read_only_pages(self):
        self.assertEqual(self.nodes.pages,{
            'status','devices','logs','learn','diagnostics','settings'})
        self.assertIn('data-page="status"',PAGE)
        self.assertIn('data-page="devices"',PAGE)
        self.assertIn('data-page="logs"',PAGE)
        self.assertIn('data-page="learn"',PAGE)
        self.assertIn('data-page="diagnostics"',PAGE)
        self.assertIn('data-page="settings"',PAGE)
        self.assertIn('Learning wizard not yet available',PAGE)
        self.assertIn('Native device library is being migrated',PAGE)
        self.assertNotIn('onclick=',PAGE)

    def test_embed_files_and_advanced_fallback(self):
        for resource in ('webui/index.html','webui/app.css','webui/app.js'):
            self.assertIn('"'+resource+'"',CMAKE)
        for route in ('root.uri="/"','advanced.uri="/advanced"',
                      'styles.uri="/app.css"','script.uri="/app.js"'):
            self.assertIn(route,MAIN)
        self.assertIn('root.handler=native_index_get',MAIN)
        self.assertIn('advanced.handler=index_get',MAIN)
        self.assertIn('if(provisioning)return index_get(req)',MAIN)
        self.assertIn('cfg.max_uri_handlers=17',MAIN)
        self.assertIn('href="/advanced"',PAGE)
        self.assertIn('/app.css',self.nodes.links)
        self.assertIn('/app.js',self.nodes.scripts)

    def test_all_ui_calls_use_existing_native_routes(self):
        routes=set(re.findall(r"api\('(/api/[^'?#]+)",SCRIPT))
        available={
            '/api/status','/api/events','/api/logs','/api/logs/config',
            '/api/diagnostics/crashes','/api/diagnostics/console',
            '/api/mqtt/status'
        }
        self.assertEqual(routes,available)
        for route in routes:
            self.assertIn(route,MAIN)
        for src in (PAGE,SCRIPT,CSS):
            self.assertNotIn('/rflink/api/',src)

        self.assertIn('fetch(path,{cache:',SCRIPT)
        self.assertIn("'X-RFLink-Update-Key':key",SCRIPT)

    def test_usable_logs_diagnosis_and_settings(self):
        for ident in ('radio-status-0','radio-status-1','radio-status-2',
                      'log-rows','log-export','log-radio','log-noise',
                      'diag-boots','diag-reset','console-output','console-key',
                      'console-connect','console-disconnect','console-clear',
                      'logs-settings-form','settings-save-status'):
            self.assertIn(ident,self.nodes.ids)
        self.assertIn('aria-label="Main navigation"',PAGE)
        self.assertIn("save logging settings",PAGE.lower())
        self.assertIn('type="password"',PAGE)
        self.assertIn("report.boot_phases",SCRIPT)
        self.assertIn("consoleBootId",SCRIPT)
        self.assertIn("r.text",SCRIPT)
        self.assertIn('eventIsVisible',SCRIPT)
        self.assertIn('eventIsVisible(event)',SCRIPT)


    def test_css_has_mobile_layout_and_independent_cards(self):
        self.assertIn('@media(max-width:750px)',CSS)
        self.assertIn('@media(max-width:440px)',CSS)
        self.assertIn('align-items:start',CSS)
        self.assertIn('.settings-stack',CSS)
        self.assertIn('overflow:auto',CSS)


if __name__=='__main__':
    unittest.main()
