"""Regression contracts for the unified full diagnostics + frozen 868 RAW CSV.

These are source/format tests; real ESP-IDF builds and physical RF sampling
must be verified separately.
"""
import csv
import io
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
WEB = (ROOT / "firmware/main/rflink_web_ota.cpp").read_text(encoding="utf-8")
UI = (ROOT / "firmware/main/webui/app.js").read_text(encoding="utf-8")
HTML = (ROOT / "firmware/main/webui/index.html").read_text(encoding="utf-8")


class FullCsvSync868Evidence(unittest.TestCase):
    def test_only_explicit_key_authorized_raw_export(self):
        handler = WEB.split("static esp_err_t logs_csv_async_get(httpd_req_t* req){", 1)[1]
        handler = handler.split("static ", 1)[0]
        self.assertIn('httpd_query_key_value(query,"sync868_raw"', handler)
        self.assertIn('strcmp(value,"1")==0', handler)
        self.assertLess(handler.index("if(include_sync868 && !key_ok(req))"),
                        handler.index("httpd_req_async_handler_begin(req,&copy)"))
        self.assertIn("csv_export_sync868.store(include_sync868", handler)

    def test_lease_immutable_and_bounded_before_streaming(self):
        impl = WEB.split("static esp_err_t logs_csv_get(httpd_req_t* req){", 1)[1]
        self.assertLess(impl.index("begin_evidence_export(&raw_capture)"),
                        impl.index("httpd_resp_send_chunk(req,kDiagnosticCsvHeader"))
        self.assertIn("SyncBenchExportLease sync_lease{nullptr}", impl)
        self.assertIn("sync_lease.receiver=sync_bench_provider", impl)
        self.assertIn("if(!raw_capture.used)", impl)
        self.assertIn("emit_full_csv_sync868_blocks(req,raw_capture)", impl)
        self.assertIn("if(include_sync868 && emit_full_csv_sync868_blocks", impl)
        self.assertIn("sync868_capture", impl)
        self.assertIn('sync868_raw_embedded', impl)
        self.assertIn("raw_requires_management_key", impl)
        self.assertIn("recorded_bits", impl)
        self.assertIn("while(log_provider && after<snapshot_end)", impl)

    def test_single_block_stream_keeps_exact_raw_evidence_fields(self):
        fn = WEB.split("static esp_err_t emit_full_csv_sync868_blocks(", 1)[1]
        fn = fn.split("#endif", 1)[0]
        for field in ("generation", "index", "epoch", "bit_count",
                      "first_us", "last_us", "discontinuity_before",
                      "window_end", "hex_msb"):
            self.assertIn(field, fn)
        self.assertIn('row[0]="sync868_bit_block"', fn)
        self.assertIn('row[4]="2"', fn)
        self.assertIn('row[21]="sync_bits_msb_hex"', fn)
        self.assertIn("copy_evidence(i,&block)", fn)
        self.assertIn("block.bit_count+7u", fn)
        self.assertIn("block.bytes[b]>>4", fn)
        self.assertIn("block.bytes[b]&0x0f", fn)
        self.assertIn("emit_diagnostic_csv_row(req,row)", fn)
        self.assertIn("receiver->end_evidence_export()", WEB)

    def test_frontend_uses_original_full_export_with_private_flag(self):
        fn = UI.split("async function exportFullCsv(event){", 1)[1]
        fn = fn.split("$('log-export').addEventListener", 1)[0]
        self.assertIn("sync868TrustedLan()", fn)
        self.assertIn("sync868Access()", fn)
        self.assertIn("if(captured.recording)", fn)
        self.assertIn("captured.recorded_bits>0", fn)
        self.assertIn("url.searchParams.set('sync868_raw','1')", fn)
        self.assertIn("fetch(exportUrl,{cache:'no-store',headers:exportHeaders})", fn)
        self.assertIn("zonder 868 RAW", fn)
        self.assertIn("Export full CSV", HTML)

    def test_26_column_contract_and_example_round_trip(self):
        header = ("record_type,sequence,uptime_ms,stream,radio,kind,code,"
                  "severity,count,value,correlation_id,flags,profile,"
                  "tx_transport,tx_stage,driver_attempted,driver_accepted,"
                  "hardware_restored,rx_restored,repeats,detail_state,"
                  "detail_type,detail_json,snapshot_json,event_utc,boot_id")
        self.assertIn("record_type,sequence,uptime_ms,stream,radio", WEB)
        self.assertIn('row[25]=std::to_string(rflink::time_boot_id())', WEB)
        fields = header.split(",")
        self.assertEqual(len(fields), 26)
        raw = {"schema": "rflink-sync-bits-v1", "generation": 3,
               "index": 0, "epoch": 1, "bit_count": 13,
               "first_us": 1000, "last_us": 1200,
               "discontinuity_before": False, "window_end": True,
               "hex_msb": "B3A8"}
        # Final byte is left-aligned, padding bits are zero.
        self.assertEqual(int(raw["hex_msb"][-2:], 16) & 0x07, 0)
        stream = io.StringIO()
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()
        row = {f: "" for f in fields}
        row.update(record_type="sync868_bit_block", radio="2",
                   detail_state="retained", detail_type="sync_bits_msb_hex",
                   detail_json=json.dumps(raw))
        writer.writerow(row)
        saved = next(csv.DictReader(io.StringIO(stream.getvalue())))
        self.assertEqual(json.loads(saved["detail_json"]), raw)


if __name__ == "__main__":
    unittest.main()
