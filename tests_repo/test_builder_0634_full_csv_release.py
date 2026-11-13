"""Regression coverage for Builder 0.6.34 full-CSV capabilities.
Later Builder releases must retain the old CSV transport; their current
version identity is verified by test_builder_version_consistency.
"""
from pathlib import Path
import re
import unittest
ROOT=Path(__file__).resolve().parents[1]
B=ROOT/"rflink_builder"

class Builder634Release(unittest.TestCase):
    def test_0634_csv_release_notes_remain_documented(self):
        # The old release remains in history, but newer Builder versions
        # are expected to advance the add-on image and UI version together.
        self.assertIn("## v0.6.34", (B/"CHANGELOG.md").read_text(encoding="utf-8"))
    def test_existing_ingress_csv_stream_with_mirrored_ui(self):
        builder=(B/"server.py").read_text(encoding="utf-8")
        proxy=(B/"device_web_proxy.py").read_text(encoding="utf-8")
        mirror=(B/"static/rflink_web/index.html").read_text(encoding="utf-8")
        app=(B/"static/rflink_web/app.js").read_text(encoding="utf-8")
        self.assertIn("open_diagnostic_csv_stream(device_book, self.path)",builder)
        self.assertIn("self._send_diagnostic_csv_stream(source)",builder)
        self.assertIn("source.read(32 * 1024)",builder)
        self.assertIn("def open_diagnostic_csv_stream(",proxy)
        self.assertIn("_verified_ip(book, device_id)",proxy)
        self.assertIn("Export full CSV",mirror)
        self.assertIn("RFLink_Diagnostics_Full.csv",mirror)
        self.assertIn("'api/logs.csv'", app)
        self.assertIn("test_full_diagnostics_csv_streams_beyond_old_six_megabyte_limit",
                      (ROOT/"tests_repo/test_ingress_http.py").read_text(encoding="utf-8"))

if __name__=="__main__":
    unittest.main()
