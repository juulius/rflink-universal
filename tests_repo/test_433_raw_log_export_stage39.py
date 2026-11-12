"""433 MHz raw RF details stay integrated with existing Logs and PSRAM.
No extra receiver task, log store, HTTP export route, or persistent flash files.
"""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[1]
def read(path):return (ROOT/path).read_text("utf-8")
class RawLogIntegration(unittest.TestCase):
    def test_rmt_worker_samples_original_before_filter(self):
        src=read("firmware/components/rflink_radio_hal/rflink_rmt_rx.cpp")
        hdr=read("firmware/components/rflink_radio_hal/include/rflink_rmt_rx.hpp")
        self.assertIn("using CaptureObserver=",hdr)
        self.assertIn("set_capture_observer(",hdr)
        self.assertIn("capture_observer_(capture_context_",src)
        self.assertLess(src.index("xQueueSend(free_slots_,&event.slot,0)"),src.index("capture_observer_(capture_context_"))
        self.assertLess(src.index("capture_observer_(capture_context_"),src.index("filter_short_pulses_inplace("))
        isr=src.split("bool IRAM_ATTR RmtPulseRx::on_rx_done(",1)[1].split("esp_err_t IRAM_ATTR RmtPulseRx::arm(",1)[0]
        self.assertNotIn("capture_observer_",isr)
        self.assertIn("cfg.mem_block_symbols=96",src)
        self.assertIn("kCaptureSlots=3",hdr)
    def test_only_existing_logging_endpoints_and_buffers(self):
        main=read("firmware/main/main.cpp")
        logs=read("firmware/main/rflink_log_store.hpp")
        store=read("firmware/main/rflink_log_details.hpp")
        web=read("firmware/main/rflink_web_ota.cpp")
        js=read("firmware/main/webui/app.js")
        html=read("firmware/main/webui/index.html")
        self.assertIn("rmt_raw_capture=46",logs)
        self.assertIn("log_rmt_raw_capture(",main)
        self.assertIn("rmt_a.set_capture_observer(",main)
        self.assertIn("rmt_b.set_capture_observer(",main)
        self.assertIn("detail_logs->save_rf(false,",main)
        self.assertIn("kPreFilterKey",main)
        self.assertIn("count<1 || count>1024",store)
        self.assertIn("requested_pre_filter",web)
        self.assertIn("raw_capture.uri=\"/api/radio/capture\"",web)
        self.assertIn("Export RF capture JSON",js)
        self.assertIn("Pulse widths (high",js)
        self.assertIn("Gap widths (low",js)
        self.assertIn("Pulse periods (high + next gap)",js)
        self.assertIn("Export RF capture JSON",html)
        self.assertIn("e.code===46",js)
        self.assertIn("rmt_raw_capture: return",web)
        self.assertNotIn('"/api/radio/export"',web)
if __name__=="__main__":unittest.main()
