import json
import pathlib
import sys
import unittest
from unittest.mock import patch

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "rflink_builder"))
import replay_export as export


class ReplayExportTests(unittest.TestCase):
    def form(self, value=None, csrf="secret"):
        if value is None:
            value = {"schema": "rflink-offline-replay-v1", "results": [],
                     "summary": {}, "notes": ["één\nregel", "a=b & c"]}
        return ("csrf=" + csrf + "\r\nreport=" +
                json.dumps(value, ensure_ascii=False, separators=(",", ":")) +
                "\r\n").encode("utf-8")

    def test_roundtrip(self):
        body = self.form()
        result = export.report_attachment(body, "secret")
        self.assertEqual(result, body.split(b"\r\n")[1][7:] + b"\n")
        self.assertEqual(json.loads(result)["notes"], ["één\nregel", "a=b & c"])

    def test_bad_csrf_and_schema(self):
        for body, status in [(self.form(csrf="wrong"), 403),
                             (self.form({"schema": "other"}), 400),
                             (self.form({"schema": "rflink-offline-replay-v1",
                                         "results": [None]*5001, "summary": {}}), 400),
                             (b"csrf=secret\r\nreport={broken}\r\n", 400),
                             (self.form()+b"filename=other\r\n", 400)]:
            with self.assertRaises(export.ExportError) as ctx:
                export.report_attachment(body, "secret")
            self.assertEqual(ctx.exception.status, status)

    def test_size_limit(self):
        with patch.object(export, "MAX_REPORT_BYTES", 10):
            with self.assertRaises(export.ExportError) as ctx:
                export.report_attachment(self.form(), "secret")
            self.assertEqual(ctx.exception.status, 413)

    def test_transport_and_manual_fallback_contract(self):
        server = (ROOT/"rflink_builder/server.py").read_text()
        ui = (ROOT/"rflink_builder/static/replay_ui.js").read_text()
        html = (ROOT/"rflink_builder/static/index.html").read_text()
        self.assertIn("report_attachment(body, CSRF)", server)
        self.assertIn("api/replay/export", ui)
        self.assertIn("form.submit()", ui)
        self.assertIn("execCommand('copy')", ui)
        self.assertIn('id="rf-replay-export-text"', html)
        self.assertNotIn("URL.createObjectURL", ui)
