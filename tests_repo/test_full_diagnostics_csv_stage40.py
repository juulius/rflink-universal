"""Full RFLink diagnostics CSV: all retained events, RF evidence, and live context."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]

def read(path):
    return (ROOT/path).read_text(encoding="utf-8")


class FullDiagnosticsCsv(unittest.TestCase):
    def test_export_ignores_every_view_filter(self):
        web = read("firmware/main/rflink_web_ota.cpp")
        chunk = web.split("static esp_err_t logs_csv_get(httpd_req_t* req){",1)[1].split(
            "static std::string log_config_snapshot_json(){",1)[0]
        self.assertIn("all.include_noise=true",chunk)
        self.assertIn("page_after(records,16,after,snapshot_end,all)",chunk)
        self.assertNotIn("log_query(req)",chunk)
        self.assertIn("snapshot_end=log_provider?log_provider->latest_sequence()",chunk)
        self.assertIn("httpd_resp_send_chunk",chunk)
        app=read("firmware/main/webui/app.js")
        html=read("firmware/main/webui/index.html")
        self.assertIn("const query=selectedLogQuery(true)",app)
        self.assertIn("$('log-export').href='/api/logs.csv'",app)
        self.assertNotIn("selectedLogQuery(false)",app)
        self.assertIn("Export full CSV",html)
        self.assertIn("regardless of the selected filters",html)

    def test_detailed_and_system_diagnostics_on_every_download(self):
        web=read("firmware/main/rflink_web_ota.cpp")
        section=web.split("static esp_err_t logs_csv_get(httpd_req_t* req){",1)[1].split(
            "static std::string log_config_snapshot_json(){",1)[0]
        self.assertIn('emit_diagnostic_csv_context(req,"device_status",',section)
        self.assertIn('status_snapshot_json()',section)
        self.assertIn('log_config_snapshot_json()',section)
        self.assertIn('rflink::crash_report_json()',section)
        self.assertIn('"export_cutoff"',section)
        self.assertIn('"export_summary"',section)
        self.assertIn("export_copy_detail(e,detail",section)
        self.assertIn("rflink::DetailSnapshot",section)
        self.assertIn("detail_snapshot_unavailable",section)
        self.assertIn("RFLink_Diagnostics_Full.csv",section)
        self.assertIn("record_type,sequence,uptime_ms,stream,radio",web)
        self.assertIn("not_retained_or_overwritten",web)
        self.assertIn("pulses_us",web)
        self.assertIn("fifo_bytes_hex",web)
        self.assertIn("quality_state",web)
        self.assertIn("frequency_offset_hz",web)
        self.assertIn("pre?(uint64_t(1)<<62):0",web)
        self.assertIn("e.code==46",web)
        self.assertIn("csv_escape_field",web)
        self.assertIn("if(c=='\"')escaped.push_back('\"')",web)

    def test_existing_log_stores_and_endpoints_stay(self):
        web=read("firmware/main/rflink_web_ota.cpp")
        for marker in ("static esp_err_t raw_capture_get(",
                       "static esp_err_t log_detail_get(",
                       "static esp_err_t log_config_get(",
                       "static esp_err_t logs_get(",
                       "static esp_err_t logs_csv_get("):
            self.assertIn(marker,web)
        self.assertIn("rmt_raw_capture=46",read("firmware/main/rflink_log_store.hpp"))

    def test_ingress_csv_is_streamed_not_buffered(self):
        proxy=read("rflink_builder/device_web_proxy.py")
        server=read("rflink_builder/server.py")
        self.assertIn("def open_diagnostic_csv_stream(",proxy)
        self.assertIn("_verified_ip(book, device_id)",proxy)
        self.assertIn("target = 'http://' + ip + '/api/logs.csv'",proxy)
        self.assertIn("def _send_diagnostic_csv_stream(",server)
        self.assertIn("while True:",server)
        self.assertIn("source.read(32 * 1024)",server)
        self.assertIn("Transfer-Encoding', 'chunked'",server)
        self.assertIn("96 * 1024 * 1024",server)
        self.assertIn("open_diagnostic_csv_stream(device_book, self.path)",server)
        self.assertIn("test_full_diagnostics_csv_streams_beyond_old_six_megabyte_limit",
                      read("tests_repo/test_ingress_http.py"))


if __name__ == "__main__":
    unittest.main()
