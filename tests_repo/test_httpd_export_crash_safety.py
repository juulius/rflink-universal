"""HTTP task stack regression after repeated post-OTA httpd panics.

Native v0.7 exp.40 was stable at boot but 7 stored panics named httpd,
followed by one wifi panic. Neither the raw core dump nor timing is available;
this guard eliminates the definite oversized HTTP stack allocation and allows
diagnosis on hardware without asserting the root cause is proven.
"""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[1]
WEB=(ROOT/"firmware/main/rflink_web_ota.cpp").read_text(encoding="utf-8")

class HttpdStackSafety(unittest.TestCase):
    def test_status_snapshot_does_not_allocate_4k_on_httpd_stack(self):
        block=WEB.split("static std::string status_snapshot_json(){",1)[1].split(
            "static esp_err_t status_get(",1)[0]
        self.assertNotIn("char body[4096]",block)
        self.assertIn("constexpr size_t kStatusJsonCapacity=8192",block)
        self.assertIn("heap_caps_calloc(1,kStatusJsonCapacity",block)
        self.assertIn("MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT",block)
        self.assertIn("MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT",block)
        self.assertIn("const int written=snprintf(body,kStatusJsonCapacity,",block)
        self.assertIn("static_cast<size_t>(written)>=kStatusJsonCapacity",block)
        self.assertIn("heap_caps_free(body)",block)
        self.assertIn("httpd_stack_min_free_bytes",block)
        self.assertIn("uxTaskGetStackHighWaterMark(nullptr)",block)
        self.assertIn("return snapshot;",block)
    def test_httpd_stack_and_export_scope(self):
        self.assertIn("cfg.stack_size=16384",WEB)
        csv=WEB.split("static esp_err_t logs_csv_get(httpd_req_t* req){",1)[1].split(
            "static std::string log_config_snapshot_json(){",1)[0]
        self.assertIn("status_snapshot_json()",csv)
        self.assertIn("all.include_noise=true",csv)
        self.assertIn("page_after(records,16,after,snapshot_end,all)",csv)
        self.assertIn("httpd_resp_send_chunk",csv)
        self.assertIn("export_copy_detail(e,detail",csv)
        # The fix does not allocate another log/queue, or broaden RF exposure.
        self.assertNotIn("log_query(req)",csv)
    def test_crash_history_keeps_original_evidence(self):
        report=(ROOT/"firmware/main/rflink_crash_report.cpp").read_text(encoding="utf-8")
        self.assertIn("esp_core_dump_get_summary",report)
        self.assertIn("snapshot.dump_pc",report)
        self.assertIn("snapshot.boot_sequence",report)

if __name__=="__main__":
    unittest.main()
