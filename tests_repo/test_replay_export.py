import json
import pathlib
import sys
import unittest
from unittest.mock import patch

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "rflink_builder"))
import replay_export as export


class ReplayExportTests(unittest.TestCase):
    def setUp(self):
        self.pending = export.PendingReports()
        self.addCleanup(self.pending.clear)

    def report(self, value=None):
        if value is None:
            value = {"schema": "rflink-offline-replay-v1", "results": [],
                     "summary": {}, "notes": ["één\nregel", "a=b & c"]}
        return json.dumps(value, ensure_ascii=False).encode("utf-8")

    def test_roundtrip_and_retry(self):
        body = self.report()
        token = self.pending.prepare(body)
        self.assertRegex(token, r"^[A-Za-z0-9_-]{43}$")
        self.assertEqual(self.pending.get(token), body)
        self.assertEqual(self.pending.get(token), body)
        self.assertIsNone(self.pending.get("invalid"))

    def test_bad_schema(self):
        for body in (self.report({"schema": "other"}), b"{broken", b"",
                     self.report({"schema": "rflink-offline-replay-v1",
                                  "results": [None]*5001, "summary": {}})):
            with self.assertRaises(export.ExportError) as ctx:
                self.pending.prepare(body)
            self.assertEqual(ctx.exception.status, 400)

    def test_size_limit(self):
        with patch.object(export, "MAX_REPORT_BYTES", 10):
            with self.assertRaises(export.ExportError) as ctx:
                self.pending.prepare(self.report())
            self.assertEqual(ctx.exception.status, 413)

    def test_bounded_and_expiring(self):
        first = self.pending.prepare(self.report())
        second = self.pending.prepare(self.report())
        third = self.pending.prepare(self.report())
        self.assertIsNone(self.pending.get(first))
        self.assertIsNotNone(self.pending.get(second))
        with patch.object(export.time, "monotonic", return_value=float("inf")):
            self.assertIsNone(self.pending.get(third))

    def test_transport_and_manual_fallback_contract(self):
        server = (ROOT/"rflink_builder/server.py").read_text()
        ui = (ROOT/"rflink_builder/static/replay_ui.js").read_text()
        html = (ROOT/"rflink_builder/static/index.html").read_text()
        self.assertIn("pending_reports.prepare(body)", server)
        self.assertIn("pending_reports.get(token)", server)
        self.assertIn("MAX_REPORT_BYTES if path == '/api/replay/export'", server)
        self.assertIn("api/replay/export", ui)
        self.assertIn("api/replay/download/", ui)
        self.assertIn("response.status", ui)
        self.assertIn("self._send_ingress_attachment(source, key, replay=True)", server)
        self.assertIn("self._send_ingress_attachment(source, progress_key)", server)
        self.assertIn("statusUrl.pathname", ui)
        self.assertIn('id="rf-replay-download-meter"', html)
        self.assertIn("execCommand('copy')", ui)
        self.assertIn('id="rf-replay-export-text"', html)
        self.assertIn('id="rf-replay-download-link"', html)
        self.assertNotIn("URL.createObjectURL", ui)
        self.assertNotIn("form.submit()", ui)
