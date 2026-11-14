"""Real HTTP regression tests for Home Assistant Ingress request framing.

These tests reproduce the uploaded HA app log:
- /wizard.js unexpectedly returns 404.
- Valid POST JSON arrives in Transfer-Encoding: chunked.
- An unconsumed chunk size (e.g. '23b', '2', '0') was then parsed as a
  separate HTTP request and logged as "Bad request syntax".
"""
from __future__ import annotations

from email.message import Message
from http.client import HTTPConnection, IncompleteRead
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


    def test_full_diagnostics_csv_streams_beyond_old_six_megabyte_limit(self):
        # Mock an already identity-verified device response. This checks the
        # real HTTP/1.1 Ingress framing, not merely source-code markers.
        from contextlib import contextmanager
        source_data = b'event,pulse\n' + b'x' * (6 * 1024 * 1024 + 1024)
        @contextmanager
        def fake_source(book, path):
            self.assertIs(book, builder_module.device_book)
            self.assertTrue(path.endswith('/api/logs.csv'))
            yield io.BytesIO(source_data)
        conn = self.connection()
        try:
            with patch.object(builder_module, 'open_diagnostic_csv_stream',
                              fake_source):
                conn.request('GET', '/rfweb/rf-0123456789ab/api/logs.csv')
                response = conn.getresponse()
                self.assertEqual(response.status, 200)
                self.assertIsNone(response.getheader('Transfer-Encoding'))
                self.assertEqual(int(response.getheader('Content-Length')), len(source_data))
                self.assertIn('RFLink_Diagnostics_Full.csv',
                              response.getheader('Content-Disposition'))
                self.assertEqual(response.read(), source_data)
                # Ensure no leaked bytes corrupt the next HA Ingress request.
                conn.request('GET', '/wizard.js')
                next_response = conn.getresponse()
                self.assertEqual(next_response.status, 200)
                next_response.read()
        finally:
            conn.close()

    def test_csv_status_tracks_preparing_delivery_and_exact_length(self):
        # A real 1.2 MiB CSV matches the previously failing mobile export
        # magnitude; check per-export status without keeping payload bytes.
        from contextlib import contextmanager
        raw = b'record_type,sequence\n' + b'"event",42\n' * 120000
        token = '0123456789abcdef01234567'
        url = '/rfweb/rf-0123456789ab/api/logs.csv?export_id=' + token
        @contextmanager
        def fake_source(book, path):
            self.assertTrue(path.endswith('?export_id=' + token))
            yield io.BytesIO(raw)
        conn = self.connection()
        try:
            with patch.object(builder_module, 'open_diagnostic_csv_stream', fake_source):
                conn.request('GET', url)
                response = conn.getresponse()
                self.assertEqual(response.status, 200)
                self.assertEqual(int(response.getheader('Content-Length')), len(raw))
                self.assertEqual(response.read(), raw)
                key=('rf-0123456789ab',token)
                state=builder_module._csv_progress_snapshot(key)
                self.assertEqual(state['state'], 'done')
                self.assertEqual(state['bytes'], len(raw))
                self.assertEqual(state['sent'], len(raw))
                self.assertEqual(state['total'], len(raw))
                self.assertEqual(state['error'], '')
                self.assertEqual(state['events'], 120000)
        finally:
            conn.close()

    def test_upstream_csv_failure_before_headers_returns_error_not_200(self):
        from contextlib import contextmanager
        token='fedcba9876543210fedcba98'
        class FailedSource:
            def __init__(self):
                self.calls=0
            def read(self, size):
                self.calls+=1
                if self.calls==1:
                    return b'record_type,sequence\\n"event",1\\n'
                raise TimeoutError('Fake upstream timeout (not exposed)')
        @contextmanager
        def broken_source(book, path):
            yield FailedSource()
        conn=self.connection()
        try:
            with patch.object(builder_module, 'open_diagnostic_csv_stream', broken_source):
                conn.request('GET', '/rfweb/rf-0123456789ab/api/logs.csv?export_id='+token)
                response=conn.getresponse()
                error=json.loads(response.read())
                self.assertEqual(response.status,504)
                self.assertEqual(error['reason'],'upstream_timeout')
                state=builder_module._csv_progress_snapshot(('rf-0123456789ab',token))
                self.assertEqual(state['state'],'failed')
                self.assertEqual(state['error'],'upstream_timeout')
                self.assertGreater(state['bytes'],0)
                self.assertEqual(state['sent'],0)
                # Keepalive remains synchronized after an upstream timeout.
                conn.request('GET','/wizard.js')
                normal=conn.getresponse()
                self.assertEqual(normal.status,200)
                normal.read()
        finally:
            conn.close()

    def test_truncated_device_chunked_http_is_an_explicit_failure(self):
        from contextlib import contextmanager
        token='aabbccddeeff001122334455'
        class TruncatedDeviceResponse:
            def __init__(self):self.calls=0
            def read(self,size):
                self.calls+=1
                if self.calls==1:return b'record_type,sequence\\n"event",1\\n'
                raise IncompleteRead(b'partial',32)
        @contextmanager
        def broken_source(book,path):
            yield TruncatedDeviceResponse()
        conn=self.connection()
        try:
            with patch.object(builder_module,'open_diagnostic_csv_stream',broken_source):
                conn.request('GET','/rfweb/rf-0123456789ab/api/logs.csv?export_id='+token)
                response=conn.getresponse()
                payload=json.loads(response.read())
                self.assertEqual(response.status,502)
                self.assertEqual(payload['reason'],'upstream_incomplete')
                progress=builder_module._csv_progress_snapshot(('rf-0123456789ab',token))
                self.assertEqual(progress['state'],'failed')
                self.assertEqual(progress['error'],'upstream_incomplete')
                self.assertEqual(progress['sent'],0)
        finally:
            conn.close()

    def test_memory_diagnostics_is_served_through_authenticated_ingress(self):
        conn = self.connection()
        try:
            conn.request("GET", "/memory_diag.js")
            res = conn.getresponse()
            script = res.read()
            self.assertEqual(res.status, 200)
            self.assertIn(b"updateMemoryDiagnostics", script)
            self.assertIn("javascript", res.getheader("Content-Type"))
            conn.request("GET", "/api/diagnostics/memory")
            res = conn.getresponse()
            report = json.loads(res.read())
            self.assertEqual(res.status, 200)
            self.assertEqual(report["interval_seconds"], 5)
            self.assertIn("last_build", report)
            self.assertIn("history", report)
            self.assertNotIn("password", json.dumps(report).lower())
        finally:
            conn.close()

    def test_idle_http_keepalive_socket_is_released(self):
        # Home Assistant Ingress may open persistent HTTP/1.1 connections.
        # This test shortens the production timeout so it finishes quickly.
        class ShortIdleHandler(builder_module.Handler):
            idle_timeout_seconds = 0.15

        srv = ThreadingHTTPServer(("127.0.0.1", 0), ShortIdleHandler)
        srv.daemon_threads = True
        thread = threading.Thread(target=srv.serve_forever, daemon=True)
        thread.start()
        conn = HTTPConnection("127.0.0.1", srv.server_port, timeout=2)
        try:
            conn.request("GET", "/memory_diag.js")
            response = conn.getresponse()
            self.assertEqual(response.status, 200)
            response.read()
            self.assertIsNotNone(conn.sock)
            conn.sock.settimeout(2)
            # The request handler must terminate an idle socket, not retain
            # its Python worker thread indefinitely.
            self.assertEqual(conn.sock.recv(1), b"")
        finally:
            conn.close()
            srv.shutdown()
            srv.server_close()
            thread.join(timeout=3)

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
            # The server deliberately rejects the oversized chunk as soon as
            # it reads its length, before reading its body. Sending all 8193
            # bytes with HTTPConnection.request() races the server's close and
            # can cause BrokenPipeError on the client's final zero chunk.
            # Send the framing prefix only: it is enough to prove that the
            # body limit is enforced and the connection is closed.
            conn.putrequest("POST", "/api/board/save")
            conn.putheader("X-RFLink-CSRF", builder_module.CSRF)
            conn.putheader("Content-Type", "application/json")
            conn.putheader("Transfer-Encoding", "chunked")
            conn.endheaders()
            conn.send(b"2001\r\n")  # 0x2001 = 8193 > MAX_JSON (8192)
            response = conn.getresponse()
            payload = json.loads(response.read())
            self.assertEqual(response.status, 413)
            self.assertIn("size limit", payload["error"])
            self.assertEqual(response.getheader("Connection"), "close")
            self.assertTrue(response.will_close)
        finally:
            conn.close()


if __name__ == "__main__":
    unittest.main()
