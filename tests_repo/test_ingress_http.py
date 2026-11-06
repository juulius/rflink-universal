"""Real HTTP regression tests for Home Assistant Ingress request framing.

These tests reproduce the uploaded HA app log:
- /wizard.js unexpectedly returns 404.
- Valid POST JSON arrives in Transfer-Encoding: chunked.
- An unconsumed chunk size (e.g. '23b', '2', '0') was then parsed as a
  separate HTTP request and logged as "Bad request syntax".
"""
from __future__ import annotations

from email.message import Message
from http.client import HTTPConnection
from http.server import ThreadingHTTPServer
import importlib.util
import io
import json
import os
from pathlib import Path
import tempfile
import threading
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
DATA = tempfile.TemporaryDirectory(prefix="rflink-ingress-http-")
os.environ["RFLINK_DATA"] = DATA.name
os.environ["RFLINK_DEV_MODE"] = "1"
spec = importlib.util.spec_from_file_location(
    "rflink_ingress_http_test", ROOT / "rflink_builder/server.py"
)
builder_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(builder_module)

from http_body import BodyReadError, read_body


def header(**values):
    msg = Message()
    for key, value in values.items():
        msg.add_header(key.replace("_", "-"), value)
    return msg


class BodyFramingTests(unittest.TestCase):
    def test_chunked_with_extension_and_trailer_consumes_complete_body(self):
        wire = io.BytesIO(b"2;ext=yes\r\n{}\r\n0\r\nX-Trace: ok\r\n\r\nGET / HTTP/1.1\r\n")
        value = read_body(header(Transfer_Encoding="chunked"), wire, 8192)
        self.assertEqual(value, b"{}")
        self.assertEqual(wire.readline(), b"GET / HTTP/1.1\r\n")

    def test_bounded_chunked_rejects_over_limit(self):
        wire = io.BytesIO(b"2001\r\n")  # 8193 bytes > 8192
        with self.assertRaises(BodyReadError) as err:
            read_body(header(Transfer_Encoding="chunked"), wire, 8192)
        self.assertEqual(err.exception.status, 413)

    def test_content_length_and_chunked_are_not_accepted_together(self):
        wire = io.BytesIO(b"2\r\n{}\r\n0\r\n\r\n")
        with self.assertRaises(BodyReadError):
            read_body(header(Transfer_Encoding="chunked", Content_Length="2"), wire, 8192)

    def test_duplicate_content_length_is_rejected(self):
        h = header(Content_Length="2")
        h.add_header("Content-Length", "3")
        with self.assertRaises(BodyReadError):
            read_body(h, io.BytesIO(b"{}"), 8192)

    def test_missing_length_means_empty_post(self):
        self.assertEqual(read_body(header(), io.BytesIO(), 8192), b"")

    def test_plain_content_length_reader_preserves_next_request(self):
        source = io.BytesIO(b"{}GET / HTTP/1.1\r\n")
        self.assertEqual(read_body(header(Content_Length="2"), source, 8192), b"{}")
        self.assertEqual(source.readline(), b"GET / HTTP/1.1\r\n")

    def test_malformed_chunk_size_is_rejected(self):
        with self.assertRaises(BodyReadError):
            read_body(header(Transfer_Encoding="chunked"), io.BytesIO(b"what\r\n"), 8192)


class LiveIngressTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.httpd = ThreadingHTTPServer(("127.0.0.1", 0), builder_module.Handler)
        cls.httpd.daemon_threads = True
        cls.thread = threading.Thread(target=cls.httpd.serve_forever, daemon=True)
        cls.thread.start()

    @classmethod
    def tearDownClass(cls):
        cls.httpd.shutdown()
        cls.httpd.server_close()
        cls.thread.join(timeout=5)
        DATA.cleanup()

    def connection(self):
        return HTTPConnection("127.0.0.1", self.httpd.server_port, timeout=5)

    def post(self, conn, url, body=None, *, chunked=True, content_type="application/json"):
        headers = {"X-RFLink-CSRF": builder_module.CSRF}
        if content_type:
            headers["Content-Type"] = content_type
        if chunked:
            conn.request("POST", url, body=iter([body] if body else []),
                         headers=headers, encode_chunked=True)
        else:
            conn.request("POST", url, body=body or b"", headers=headers)
        res = conn.getresponse()
        data = res.read()
        return res.status, json.loads(data)

    def test_new_wizard_javascript_is_served(self):
        conn = self.connection()
        try:
            conn.request("GET", "/wizard.js")
            res = conn.getresponse()
            content = res.read()
            self.assertEqual(res.status, 200)
            self.assertIn(b"beginWizard", content)
            self.assertIn("javascript", res.getheader("Content-Type"))
        finally:
            conn.close()

    def test_chunked_board_save_succeeds_and_keeps_connection_in_sync(self):
        conn = self.connection()
        try:
            with patch.object(builder_module.local_profile, "save", return_value={"ok": True}) as save:
                status, payload = self.post(conn, "/api/board/save", b'{"board":{}}')
                self.assertEqual(status, 200)
                self.assertTrue(payload["ok"])
                save.assert_called_once_with({})
            conn.request("GET", "/wizard.js")
            res = conn.getresponse()
            self.assertEqual(res.status, 200)
            res.read()
        finally:
            conn.close()

    def test_empty_chunked_discovery_does_not_leave_a_2_or_0_marker(self):
        conn = self.connection()
        try:
            with patch.object(builder_module.device_book, "discover", return_value={"started": True}):
                status, result = self.post(conn, "/api/devices/discover", b"{}")
                self.assertEqual(status, 202)
            conn.request("GET", "/devices.js")
            res = conn.getresponse()
            self.assertEqual(res.status, 200)
            res.read()
        finally:
            conn.close()

    def test_chunked_password_save_is_fully_read_before_validation(self):
        conn = self.connection()
        try:
            with patch.object(builder_module.local_secrets, "save", return_value={"ok": True}):
                status, payload = self.post(conn, "/api/secrets/save", b'{"wifi_ssid":"Test"}')
                self.assertEqual(status, 200)
            conn.request("GET", "/wizard.js")
            response = conn.getresponse()
            self.assertEqual(response.status, 200)
            response.read()
        finally:
            conn.close()

    def test_chunked_build_request_succeeds_without_stray_zero(self):
        conn = self.connection()
        try:
            builder_module.builder = type("FakeBuilder", (), {"start": lambda self: "test-job"})()
            status, result = self.post(conn, "/api/build", b"{}")
            self.assertEqual(status, 202)
            self.assertEqual(result["job_id"], "test-job")
            conn.request("GET", "/wizard.js")
            res = conn.getresponse()
            self.assertEqual(res.status, 200)
            res.read()
        finally:
            conn.close()

    def test_oversized_chunked_json_yields_413_and_closes_socket(self):
        conn = self.connection()
        try:
            status, payload = self.post(conn, "/api/board/save", b"x" * 8193)
            self.assertEqual(status, 413)
            self.assertIn("size limit", payload["error"])
        finally:
            conn.close()


if __name__ == "__main__":
    unittest.main()
