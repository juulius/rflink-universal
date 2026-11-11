"""RFLink web UI via Home Assistant Ingress: origin and device security tests."""
from __future__ import annotations
from pathlib import Path
import threading
import unittest
from unittest.mock import patch

from device_web_proxy import handle_request, ProxyRequestError, _VERIFIED

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / 'rflink_builder/static/rflink_web'
ID = 'rf-68ee8f479058'
IP = '192.168.52.67'


class Book:
    def __init__(self):
        self.lock = threading.RLock()
        self.devices = {ID: {
            'id': ID, 'name': 'RFLink', 'ip': IP, 'online': True,
            'managed': True, 'identity_verified': True,
            'physical_id': ID, 'board_id': 'rflink-s3-n16r8-reva',
            'board_hash': 'b5e871b06',
        }}


class FakeResponse:
    status = 200
    def __init__(self, content=b'{"firmware":"0.7.0"}'):
        self.content = content
    def __enter__(self): return self
    def __exit__(self, *_): pass
    def read(self, amount): return self.content[:amount]


class FakeOpener:
    def __init__(self):
        self.requests = []
    def open(self, request, timeout=None):
        self.requests.append(request)
        return FakeResponse()


class IngressDeviceWebTests(unittest.TestCase):
    def setUp(self):
        _VERIFIED.clear()
        self.book = Book()
        self.url = '/rfweb/' + ID + '/'
        self.expected = {'id': ID, 'board_id': 'rflink-s3-n16r8-reva',
                         'board_hash': 'b5e871b06'}

    def call(self, path='', method='GET', body=b'', headers=None):
        return handle_request(self.book, self.url + path, method, 'TEST_CSRF',
                              body=body, headers=headers, asset_root=ASSETS)

    def test_builder_serves_trusted_relative_assets_not_device_supplied_html(self):
        with patch('device_web_proxy._probe_opener') as opened:
            status, html, kind = self.call()
            self.assertEqual(status, 200)
            self.assertIn(b'content="TEST_CSRF"', html)
            self.assertIn(b'href="app.css"', html)
            self.assertIn(b'src="app.js"', html)
            self.assertNotIn(b'href="/app.css"', html)
            self.assertEqual(self.call('app.css')[0], 200)
            _, script, mime = self.call('app.js')
            self.assertIn(b'X-RFLink-CSRF', script)
            self.assertIn(b"api/logs", script)
            self.assertNotIn(b"'/api/", script)
            opened.assert_not_called()

    def test_read_proxy_uses_only_saved_private_ip_and_verifies_identity(self):
        opened = FakeOpener()
        with patch('device_web_proxy.probe', return_value=self.expected), \
             patch('device_web_proxy._probe_opener', return_value=opened):
            status, body, kind = self.call('api/status?unused=1')
        self.assertEqual(status, 200)
        self.assertEqual(kind, 'application/json; charset=utf-8')
        self.assertEqual(len(opened.requests), 1)
        self.assertEqual(opened.requests[0].full_url,
                         'http://' + IP + '/api/status?unused=1')

    def test_write_requires_exact_physical_device_even_on_saved_ip(self):
        with patch('device_web_proxy.probe', return_value={
            **self.expected, 'id': 'rf-123456abcdef'
        }), patch('device_web_proxy._probe_opener') as opened:
            with self.assertRaisesRegex(ProxyRequestError, 'identity changed') as error:
                self.call('api/led', 'POST', b'{"enabled":true}',
                          {'Content-Type': 'application/json'})
            self.assertEqual(error.exception.status, 409)
            opened.assert_not_called()

    def test_post_rejects_unsafe_endpoints_before_network(self):
        with patch('device_web_proxy.probe') as checked:
            for path in ('api/status', '../api/led', 'app.js', 'api/ota?debug=1',
                         'http://127.0.0.1/', 'api/not-a-real-route'):
                with self.subTest(path=path), self.assertRaises(ProxyRequestError):
                    self.call(path, 'POST', b'{}', {'Content-Type': 'application/json'})
            checked.assert_not_called()

    def test_secrets_are_not_automatically_forwarded(self):
        opened = FakeOpener()
        with patch('device_web_proxy.probe', return_value=self.expected), \
             patch('device_web_proxy._probe_opener', return_value=opened):
            self.call('api/led', 'POST', b'{"enabled":true}',
                      {'Content-Type': 'application/json', 'Authorization': 'test'})
        request = opened.requests[0]
        self.assertIsNone(request.get_header('Authorization'))
        self.assertIsNone(request.get_header('X-rflink-update-key'))

    def test_reject_ip_bound_unmanaged_and_public_devices(self):
        for update in ({'managed': False}, {'identity_verified': False},
                       {'ip': '8.8.8.8'}, {'online': False}):
            with self.subTest(update=update):
                _VERIFIED.clear()
                self.book.devices[ID].update(update)
                with self.assertRaises(ProxyRequestError):
                    self.call('api/status')
                self.book = Book()

    def test_ota_needs_explicit_password_and_binary(self):
        with patch('device_web_proxy.probe', return_value=self.expected), \
             patch('device_web_proxy._probe_opener', return_value=FakeOpener()):
            with self.assertRaises(ProxyRequestError):
                self.call('api/ota', 'POST', b'123', {'Content-Type': 'application/octet-stream'})
            status, _, _ = self.call('api/ota', 'POST', b'123',
                {'Content-Type': 'application/octet-stream',
                 'X-RFLink-Update-Key': 'temporary-test-password'})
            self.assertEqual(status, 200)


if __name__ == '__main__':
    unittest.main()
