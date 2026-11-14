"""RF Replay live RFLink snapshot: bounded import, identity and UI gates."""
from __future__ import annotations

import csv
from contextlib import contextmanager
import importlib.util
import io
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import threading
import unittest
from http.client import HTTPConnection
from http.server import ThreadingHTTPServer
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "rflink_builder"))
import replay_service


def sample_csv():
    output = io.StringIO(newline="")
    fields = ["record_type", "detail_state", "detail_json", "detail_type",
              "radio", "code", "boot_id", "sequence", "event_utc", "uptime_ms"]
    writer = csv.DictWriter(output, fieldnames=fields)
    writer.writeheader()
    writer.writerow({
        "record_type": "event", "detail_state": "retained",
        "detail_type": "signed_pulses_us", "radio": "1", "code": "46",
        "boot_id": "12", "sequence": "42",
        "event_utc": "2026-11-14T12:00:00.000Z", "uptime_ms": "2000",
        "detail_json": json.dumps({"radio": 1, "frequency_hz": 433920000,
                                   "format": "signed_pulses_us",
                                   "count": 2, "pulses_us": [320, -520]}),
    })
    return output.getvalue().encode("utf-8")


class LiveReplayServiceTests(unittest.TestCase):
    def test_live_capture_uses_existing_decoder_without_persisting_raw_rf(self):
        body = sample_csv()
        visited = []
        normal_load = replay_service.rf_replay.load_captures

        def observed(path):
            visited.append(path)
            self.assertTrue(path.exists())
            return normal_load(path)

        @contextmanager
        def open_source():
            with io.BytesIO(body) as stream:
                yield stream

        with patch.object(replay_service.rf_replay, "load_captures", side_effect=observed):
            result = replay_service.analyze_device_stream(open_source)
        self.assertEqual(result["schema"], "rflink-offline-replay-v1")
        self.assertEqual(result["capture_count"], 1)
        self.assertEqual(result["summary"]["radio_1"], 1)
        self.assertEqual(result["limits"]["source"], "live_device")
        self.assertEqual(result["limits"]["input_bytes"], len(body))
        self.assertFalse(result["limits"]["raw_captures_persisted"])
        self.assertNotIn('"pulses_us"', json.dumps(result))
        self.assertNotIn('"hex"', json.dumps(result))
        self.assertTrue(visited)
        self.assertFalse(visited[0].exists())

    def test_live_import_is_singleflight(self):
        replay_service._REPLAY_SLOT.acquire()
        try:
            with self.assertRaises(replay_service.ReplayBusy):
                replay_service.analyze_device_stream(lambda: None)
        finally:
            replay_service._REPLAY_SLOT.release()

    def test_empty_or_oversize_stream_is_rejected_and_closed(self):
        for content in (b"", b"x" * 2048):
            closed = []
            @contextmanager
            def source():
                try:
                    with io.BytesIO(content) as data:
                        yield data
                finally:
                    closed.append(True)
            with patch.object(replay_service, "MAX_LIVE_EXPORT_BYTES", 1024):
                with self.assertRaises(replay_service.rf_replay.ReplayError):
                    replay_service.analyze_device_stream(source)
            self.assertEqual(closed, [True])

    def test_device_route_uses_only_allowlisted_identity_verified_proxy(self):
        code = (ROOT/"rflink_builder/server.py").read_text()
        proxy = (ROOT/"rflink_builder/device_web_proxy.py").read_text()
        self.assertIn("if path == '/api/replay/device':", code)
        self.assertIn("set(request) != {'device_id'}", code)
        self.assertIn("open_diagnostic_csv_stream(", code)
        self.assertIn("analyze_live_rf_replay(open_verified_snapshot)", code)
        self.assertIn("if not self._authorized(write=True):", code)
        self.assertIn("ip = _verified_ip(book, device_id)", proxy)
        self.assertIn("resource != 'api/logs.csv'", proxy)

    @unittest.skipUnless(shutil.which("node"), "Node.js required")
    def test_live_page_script_is_valid_and_has_manual_import_fallback(self):
        ui = ROOT/"rflink_builder/static/replay_ui.js"
        parsed = subprocess.run(["node", "--check", str(ui)],
                                capture_output=True, text=True, timeout=20)
        self.assertEqual(parsed.returncode, 0, parsed.stderr)
        source = ui.read_text()
        html = (ROOT/"rflink_builder/static/index.html").read_text()
        for marker in ("rf-replay-live-device", "rf-replay-live-a",
                       "rf-replay-live-b", "rf-replay-live-status"):
            self.assertIn('id="'+marker+'"', html)
        self.assertIn("api/replay/device", source)
        self.assertIn("api/devices", source)
        self.assertIn("identity_verified===true", source)
        self.assertIn("'X-RFLink-CSRF':csrf", source)
        self.assertIn("function importFile(", source)
        self.assertNotIn("innerHTML", source)


class LiveReplayHttpTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = tempfile.TemporaryDirectory(prefix="rf-replay-server-")
        os.environ["RFLINK_DATA"] = cls.data.name
        os.environ["RFLINK_DEV_MODE"] = "1"
        spec = importlib.util.spec_from_file_location(
            "rflink_live_replay_test_server", ROOT / "rflink_builder/server.py")
        cls.server = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(cls.server)
        cls.httpd = ThreadingHTTPServer(("127.0.0.1", 0), cls.server.Handler)
        cls.httpd.daemon_threads = True
        cls.thread = threading.Thread(target=cls.httpd.serve_forever, daemon=True)
        cls.thread.start()

    @classmethod
    def tearDownClass(cls):
        cls.httpd.shutdown()
        cls.httpd.server_close()
        cls.thread.join(timeout=5)
        cls.data.cleanup()

    def post(self, data, csrf=True):
        conn = HTTPConnection("127.0.0.1", self.httpd.server_port, timeout=8)
        try:
            headers = {"Content-Type": "application/json"}
            if csrf:
                headers["X-RFLink-CSRF"] = self.server.CSRF
            conn.request("POST", "/api/replay/device", json.dumps(data).encode(), headers)
            response = conn.getresponse()
            return response.status, json.loads(response.read())
        finally:
            conn.close()

    def test_reject_untrusted_device_names_and_missing_csrf(self):
        for data in ({"device_id": "192.168.52.67"},
                     {"device_id": "rf-0123456789ab", "url": "http://example.com"},
                     {"device_id": "../other"},
                     {"device_id": 42}):
            code, _ = self.post(data)
            self.assertEqual(code, 400)
        code, _ = self.post({"device_id": "rf-0123456789ab"}, csrf=False)
        self.assertEqual(code, 403)

    def test_server_passes_fixed_device_path_without_browser_download(self):
        observed = []
        @contextmanager
        def source(book, raw_path):
            observed.append((book, raw_path))
            with io.BytesIO(sample_csv()) as body:
                yield body
        with patch.object(self.server, "open_diagnostic_csv_stream", source):
            code, report = self.post({"device_id": "rf-0123456789ab"})
        self.assertEqual(code, 200, report)
        self.assertEqual(report["capture_count"], 1)
        self.assertEqual(report["limits"]["source"], "live_device")
        self.assertEqual(observed[0][0], self.server.device_book)
        self.assertEqual(observed[0][1], "/rfweb/rf-0123456789ab/api/logs.csv")


if __name__ == "__main__":
    unittest.main()
