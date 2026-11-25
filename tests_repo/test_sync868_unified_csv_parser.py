"""The offline 868 parser accepts full diagnostics CSV with embedded RAW bits."""
from __future__ import annotations

import csv
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "firmware" / "tools"))
from inspect_sync_bits import read_capture, segments, summarize  # noqa: E402

HEADER = ("record_type,sequence,uptime_ms,stream,radio,kind,code,"
          "severity,count,value,correlation_id,flags,profile,tx_transport,"
          "tx_stage,driver_attempted,driver_accepted,hardware_restored,"
          "rx_restored,repeats,detail_state,detail_type,detail_json,"
          "snapshot_json,event_utc,boot_id").split(",")


def make_csv(*, enabled=True, corrupt=None):
    stream = io.StringIO()
    writer = csv.DictWriter(stream, fieldnames=HEADER, lineterminator="\n")
    writer.writeheader()

    def row(**kwargs):
        value = {name: "" for name in HEADER}
        value.update(kwargs)
        writer.writerow(value)

    row(record_type="context", kind="export_schema",
        snapshot_json=json.dumps({"name": "rflink-diagnostics-csv-v2"}))
    context = {"mode": "experimental", "recording": False, "generation": 5,
               "full": False, "available_blocks": 2, "recorded_bits": 21,
               "raw_embedded": enabled, "raw_requires_management_key": True}
    row(record_type="context", kind="sync868_capture",
        snapshot_json=json.dumps(context))
    row(record_type="event", stream="radio", radio="2", kind="noise")
    if enabled:
        for index, (bits, hex_payload) in enumerate([(13, "F0F8"), (8, "A5")]):
            raw = {"schema": "rflink-sync-bits-v1", "generation": 5,
                   "index": index, "epoch": 2, "bit_count": bits,
                   "first_us": 123456 + index * 1000,
                   "last_us": 123756 + index * 1000,
                   "discontinuity_before": index == 0,
                   "window_end": index == 1, "hex_msb": hex_payload}
            if corrupt == "generation" and index == 1:
                raw["generation"] = 6
            if corrupt == "padding" and index == 0:
                raw["hex_msb"] = "F0FF"
            if corrupt == "duplicate_index" and index == 1:
                raw["index"] = 0
            row(record_type="sync868_bit_block", radio="2",
                detail_type="sync_bits_msb_hex", detail_state="retained",
                detail_json=json.dumps(raw))
    row(record_type="context", kind="export_summary",
        snapshot_json=json.dumps({"sync868_raw_embedded": enabled}))
    return stream.getvalue()


class UnifiedCsvParserTests(unittest.TestCase):
    def parse(self, value):
        with tempfile.TemporaryDirectory() as work:
            path = Path(work) / "RFLink_Diagnostics_Full.csv"
            path.write_text(value, encoding="utf-8")
            return read_capture(path)

    def test_roundtrip_and_segment_boundaries(self):
        meta, blocks = self.parse(make_csv())
        self.assertEqual(meta["schema"], "rflink-sync-bits-v1")
        self.assertEqual(meta["blocks"], "2")
        self.assertEqual(meta["recorded_bits"], "21")
        self.assertEqual([b.hex_msb for b in blocks], ["F0F8", "A5"])
        self.assertEqual(sum(b.bit_count for b in blocks), 21)
        self.assertEqual(len(segments(blocks)), 1)
        self.assertIn("recorded_bits", summarize(meta, blocks, segments(blocks)))

    def test_keyless_export_not_mistaken_for_no_rf_traffic(self):
        with self.assertRaisesRegex(ValueError, "no protected 868 RAW bits"):
            self.parse(make_csv(enabled=False))

    def test_generation_change_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "generation changed"):
            self.parse(make_csv(corrupt="generation"))

    def test_padding_and_order_are_rejected(self):
        with self.assertRaisesRegex(ValueError, "padding bits"):
            self.parse(make_csv(corrupt="padding"))
        with self.assertRaisesRegex(ValueError, "Out-of-order"):
            self.parse(make_csv(corrupt="duplicate_index"))


if __name__ == "__main__":
    unittest.main()
