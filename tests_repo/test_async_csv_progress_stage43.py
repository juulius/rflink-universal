"""Regression contract for non-blocking CSV diagnostics export."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
def source(path):
    return (ROOT / path).read_text(encoding='utf-8')

class CsvAsyncProgressTests(unittest.TestCase):
    def test_async_export_does_not_hold_main_httpd_task(self):
        cpp = source("firmware/main/rflink_web_ota.cpp")
        self.assertIn("static esp_err_t logs_csv_get(httpd_req_t* req){", cpp)
        self.assertIn("httpd_req_async_handler_begin(req,&copy)", cpp)
        self.assertIn("xTaskCreate(csv_export_task,", cpp)
        self.assertIn("httpd_req_async_handler_complete(req)", cpp)
        self.assertIn("httpd_req_async_handler_complete(copy)", cpp)
        self.assertIn("csv_export_busy.compare_exchange_strong", cpp)
        self.assertIn("csv_export_busy.store(false", cpp)
        self.assertIn("logs_csv.handler=logs_csv_async_get;", cpp)
        self.assertIn("if(result!=ESP_OK)",cpp)
        self.assertIn('X-RFLink-Export-Estimated-Events',cpp)
        self.assertIn('for(const auto used:usage.used)estimated_events+=used',cpp)
        # Still iterates the same six rings without noise/filter losses.
        self.assertIn('all.include_noise=true',cpp)
        self.assertIn('page_after(records,16,after,snapshot_end,all)',cpp)
        self.assertIn('emit_diagnostic_csv_row(req,row)',cpp)
        self.assertIn('RFLink_Diagnostics_Full.csv',cpp)
        stub=source("firmware/tests/host_idf_stubs/esp_http_server.h")
        self.assertIn('httpd_req_async_handler_begin',stub)
        self.assertIn('httpd_req_async_handler_complete',stub)

    def test_both_web_uis_stream_progress_without_blocking_navigation(self):
        for folder in ("firmware/main/webui", "rflink_builder/static/rflink_web"):
            app=source(folder+"/app.js")
            html=source(folder+"/index.html")
            css=source(folder+"/app.css")
            for key in ("log-export-progress","log-export-meter","log-export-state",
                        "log-export-feedback","log-export-percent"):
                self.assertIn('id="'+key+'"',html)
            self.assertIn('csvExportRunning',app)
            self.assertIn("event.preventDefault()",app)
            self.assertIn("meter.removeAttribute('value')",app)
            self.assertIn("csvExportRunning=false",app)
            self.assertIn("'log_export'",app)
            self.assertIn('csvExportHideTimer=setTimeout(',app)
            self.assertIn('},5000)',app)
            self.assertIn('.log-export-progress[hidden]',css)
            if folder == "firmware/main/webui":
                self.assertIn('response.body.getReader()',app)
                self.assertIn("link.href,{cache:'no-store'}",app)
                self.assertIn("completedRows/rowEstimate",app)
                self.assertIn("new Blob(chunks",app)
                self.assertIn("URL.createObjectURL(blob)",app)
                self.assertIn("URL.revokeObjectURL(url)",app)
            else:
                self.assertIn("api/logs/export-status",app)
                self.assertIn("Content-Disposition",app)
                self.assertIn("downloadUrl.searchParams.set('export_id',token)",app)
                self.assertIn("const progress=await response.json()",app)
                self.assertIn("completed/estimate*89",app)
                self.assertIn("progress.sent",app)
                self.assertIn("progress.total",app)
                csv_block=app.split("function exportFullCsv(event)",1)[1].split("$('log-export').addEventListener",1)[0]
                self.assertNotIn("URL.createObjectURL(blob)",csv_block)
                # Individual RF capture JSON can still use a Blob download.

    def test_ingress_prepares_spooled_fixed_length_attachment(self):
        proxy=source("rflink_builder/device_web_proxy.py")
        server=source("rflink_builder/server.py")
        self.assertIn("open_diagnostic_csv_stream(",proxy)
        self.assertIn("self.send_header('Content-Length', str(total))",server)
        self.assertIn("tempfile.TemporaryFile(mode='w+b'",server)
        self.assertIn("spool.close()",server)
        self.assertNotIn("Transfer-Encoding', 'chunked'",server)
        self.assertIn("getattr(source, 'headers', None)",server)
        self.assertIn("'X-RFLink-Export-Estimated-Events', estimate",server)
        self.assertIn("while True:",server)
        self.assertIn("source.read(32 * 1024)",server)
        self.assertIn("state='preparing'",server)
        self.assertIn("state='streaming'",server)
        self.assertIn("error='",server)


if __name__=="__main__":
    unittest.main()
