import importlib.util
import io
import json
import os
from pathlib import Path
import sys
import tempfile
import time
import threading
import unittest
import urllib.request
import urllib.error
import zipfile

ROOT = Path(__file__).resolve().parents[1]
SERVER_PATH = ROOT / 'rflink_builder' / 'server.py'
TD = tempfile.TemporaryDirectory()
os.environ['RFLINK_DATA'] = TD.name
os.environ['RFLINK_DEV_MODE'] = '1'
os.environ['RFLINK_PORT'] = '0'
spec = importlib.util.spec_from_file_location('rf_builder', SERVER_PATH)
mod = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = mod
spec.loader.exec_module(mod)

class BuilderTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        mod.builder = mod.Builder()
        cls.httpd = mod.ThreadingHTTPServer(('127.0.0.1', 0), mod.Handler)
        cls.server_thread = threading.Thread(target=cls.httpd.serve_forever, daemon=True)
        cls.server_thread.start()
        cls.url = f'http://127.0.0.1:{cls.httpd.server_port}/'

    @classmethod
    def tearDownClass(cls):
        cls.httpd.shutdown()
        cls.httpd.server_close()
        TD.cleanup()

    def zip_with(self, additions):
        source = (ROOT / 'rflink_builder/default_source.zip').read_bytes()
        # Add files with malicious entries to a valid ZIP.
        content = io.BytesIO()
        with zipfile.ZipFile(io.BytesIO(source)) as original, zipfile.ZipFile(content, 'w') as dest:
            for item in original.infolist():
                if not item.is_dir():
                    dest.writestr(item.filename, original.read(item))
            for name, data in additions.items():
                dest.writestr(name, data)
        return content.getvalue()

    def request(self, route, data=None, headers=None):
        req = urllib.request.Request(self.url + route, data=data, headers=headers or {})
        try:
            with urllib.request.urlopen(req, timeout=10) as r:
                return r.status, r.read(), r.headers
        except urllib.error.HTTPError as e:
            return e.code, e.read(), e.headers

    def test_initial_default_project_is_loaded(self):
        self.assertTrue((mod.PROJECT/'CMakeLists.txt').is_file())
        status, body, _ = self.request('api/status')
        self.assertEqual(status, 200)
        self.assertIn('Radioactive Stage 20', json.loads(body)['source']['name'])

    def test_relative_web_assets_and_csrf(self):
        status, html, _ = self.request('')
        self.assertEqual(status, 200)
        self.assertIn(mod.CSRF.encode(), html)
        self.assertIn(b'href="app.css"', html)
        self.assertNotIn(b'href="/app.css"', html)
        self.assertEqual(self.request('app.js')[0], 200)
        icon_status, icon_body, icon_headers = self.request('icon.png')
        self.assertEqual(icon_status, 200)
        self.assertEqual(icon_body[:8], b'\\x89PNG\\r\\n\\x1a\\n')
        self.assertEqual(icon_headers.get_content_type(), 'image/png')

    def test_management_routes_read_only_status(self):
        for route in ('api/deploy/status','api/serial','api/mqtt','api/deploy/log','api/toolchain/status','api/toolchain/log'):
            status, body, _ = self.request(route)
            self.assertEqual(status, 200, (route, body))
            self.assertIsInstance(json.loads(body), dict)

    def test_management_writes_require_csrf(self):
        routes = ('api/deploy/ota','api/deploy/usb','api/mqtt/save','api/mqtt/test','api/mqtt/reset','api/mqtt/push','api/source/reset','api/toolchain/retry')
        for route in routes:
            with self.subTest(route=route):
                status, _, _ = self.request(route, b'{}', {'Content-Type': 'application/json'})
                self.assertEqual(status, 403)

    def test_deploy_ota_requires_built_firmware(self):
        payload = json.dumps({'ip':'192.168.1.90','key':'dummy-password','confirm':'TESTBOARD'}).encode()
        status, body, _ = self.request('api/deploy/ota', payload, {
            'Content-Type':'application/json','X-RFLink-CSRF':mod.CSRF})
        self.assertEqual(status,409,body)
        self.assertIn(b'build',body)

    def test_upload_requires_csrf(self):
        status, body, _ = self.request('api/upload', b'zip', {'Content-Type':'application/zip'})
        self.assertEqual(status, 403)

    def test_uploaded_source_roundtrip(self):
        raw = (ROOT/'rflink_builder/default_source.zip').read_bytes()
        status, body, _ = self.request('api/upload', raw, {
            'Content-Type':'application/zip','X-RFLink-CSRF':mod.CSRF,'X-RFLink-Filename':'new.zip'})
        self.assertEqual(status, 200, body)
        self.assertEqual(json.loads(body)['source']['files'] > 50, True)
        self.assertTrue((mod.PROJECT/'boards/rflink_s3_n16r8.json').is_file())

    def test_zip_slip_blocked(self):
        bad = self.zip_with({'../../escape.txt': b'evil'})
        with self.assertRaisesRegex(ValueError, 'Invalid ZIP path'):
            mod.inspect_zip(bad)
        self.assertFalse((mod.DATA.parent/'escape.txt').exists())

    def test_absolute_path_blocked(self):
        bad = self.zip_with({'/escape.txt': b'evil'})
        with self.assertRaises(ValueError):
            mod.inspect_zip(bad)

    def test_symlink_blocked(self):
        info = zipfile.ZipInfo('link')
        info.create_system=3
        info.external_attr=(0o120777<<16)
        buf=io.BytesIO()
        with zipfile.ZipFile(buf,'w') as z:
            z.writestr(info,'../../escape')
        with self.assertRaisesRegex(ValueError, 'Symbolic links'):
            mod.inspect_zip(buf.getvalue())

    def test_uncompressed_limit_blocked(self):
        info = zipfile.ZipInfo('huge.txt')
        info.file_size=mod.MAX_ENTRY+1
        # Mocked size test of zip members: host safety envelope.
        buf=io.BytesIO()
        with zipfile.ZipFile(buf, 'w', compression=zipfile.ZIP_DEFLATED) as z:
            z.writestr('huge.txt', 'x' * (mod.MAX_ENTRY+1))
        with self.assertRaisesRegex(ValueError, 'too large'):
            mod.inspect_zip(buf.getvalue())

    def test_download_unavailable_without_real_build(self):
        self.assertEqual(self.request('api/download/ota')[0], 404)
        self.assertEqual(self.request('api/download/usb')[0], 404)

    def test_launch_without_idf_reports_failure(self):
        # Verify a missing compiler does not create a fake release.
        old_path = os.environ.get('PATH','')
        try:
            os.environ['PATH'] = '/usr/bin:/bin'
            mod.builder.start()
            for _ in range(100):
                if mod.builder.get_status()['status'] != 'running':
                    break
                time.sleep(.03)
            current=mod.builder.get_status()
            self.assertEqual(current['status'], 'failed')
            self.assertFalse(current['ota_ready'])
            self.assertFalse(current['bundle_ready'])
        finally:
            os.environ['PATH'] = old_path

if __name__=='__main__':
    unittest.main(verbosity=2)
