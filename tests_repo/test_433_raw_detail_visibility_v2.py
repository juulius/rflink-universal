"""Retained 433 RAW pulse evidence and truthful capture summary UX."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]

def read(path):
    return (ROOT / path).read_text(encoding="utf-8")

class RawPulseVisibility(unittest.TestCase):
    def test_forwarded_433_decoder_details_are_per_band_not_shared_noise(self):
        main = read("firmware/main/main.cpp")
        section = main.split("static void log_rx_detail(", 1)[1].split(
            "static void log_rmt_raw_capture(", 1)[0]
        self.assertIn("const bool isolated_433_decoder_pulses=pulses!=nullptr && id<2;", section)
        self.assertIn("details->save_rf(noise && !isolated_433_decoder_pulses,", section)
        self.assertIn("(uint64_t(1)<<63)|rx_seq", section)
        # The original pre-filter evidence is still captured BEFORE filtering,
        # using a distinct high-bit key in a per-radio PSRAM arena.
        self.assertIn("kPreFilterKey|seq", main)
        self.assertIn("detail_logs->save_rf(false,", main)
        receiver = read("firmware/components/rflink_radio_hal/rflink_rmt_rx.cpp")
        self.assertLess(receiver.index("capture_observer_(capture_context_"),
                        receiver.index("filter_short_pulses_inplace("))
        details = read("firmware/main/rflink_log_details.hpp")
        self.assertIn("kMaxBudgetBytes=2457*1024", details)
        self.assertIn("kArenaKiB={{512,512,256,192,384,256,256}}", details)
        self.assertIn("kEntryLimits={{250,250,250,2000,512,512,1500}}", details)

    def test_ui_explains_aggregate_vs_individual_capture_and_short_pulses(self):
        native = read("firmware/main/webui/app.js")
        ingress = read("rflink_builder/static/rflink_web/app.js")
        for script in (native, ingress):
            self.assertIn("aggregate counter, not a linked RF message", script)
            self.assertIn("if(e.code!==35 && bucket>=0", script)
            self.assertIn("((e.code>=1&&e.code<=6)||e.code===46)", script)
            self.assertIn("Enable Include noise for retained 1–2 pulse captures", script)
            self.assertIn("One or two durations cannot establish reliable bits", script)
            self.assertIn("Number(e.count)<=2?'1–2 durations", script)
            self.assertIn("GDO2 PRE-FILTER capture", script)
            self.assertIn("frame.pulses_us.join(', ')", script)
        self.assertIn("api('/api/radio/capture?radio='", native)
        self.assertIn("api('api/radio/capture?radio='", ingress)

    @unittest.skipUnless(shutil.which("g++"), "g++ required")
    def test_psram_retention_isolation_host_cpp(self):
        with tempfile.TemporaryDirectory(prefix="rflink-raw-433-") as temp:
            exe = str(Path(temp) / "test_log_details")
            built = subprocess.run(
                ["g++", "-std=c++17", "-O2", "-pthread", "-Wall",
                 "-Wextra", "-Werror", "-I"+str(ROOT/"firmware/main"),
                 str(ROOT/"firmware/tests/test_log_details.cpp"), "-o", exe],
                capture_output=True, text=True, timeout=40
            )
            self.assertEqual(built.returncode, 0, built.stderr)
            result = subprocess.run([exe], capture_output=True, text=True, timeout=20)
            self.assertEqual(result.returncode, 0, result.stderr)

if __name__ == "__main__":
    unittest.main()
