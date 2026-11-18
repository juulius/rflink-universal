"""Offline and firmware source contracts for opt-in 868 synchronous evidence.

No RF hardware, access keys, Wi-Fi, TX or live decoder are exercised here.
"""
import importlib.util
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT/"firmware/tools/inspect_sync_bits.py"
FW = ROOT/"firmware"
HAL = FW/"components/rflink_radio_hal"
WEB = FW/"main/rflink_web_ota.cpp"

spec = importlib.util.spec_from_file_location("inspect_sync_bits", SCRIPT)
module = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = module
spec.loader.exec_module(module)


class SyncEvidenceCsvTests(unittest.TestCase):
    def _csv(self, payload: str) -> Path:
        path = Path(self.temp)/"capture.csv"
        path.write_text(payload, encoding="utf-8")
        return path

    def setUp(self):
        self.workspace = tempfile.TemporaryDirectory()
        self.temp = self.workspace.name

    def tearDown(self):
        self.workspace.cleanup()

    def test_non_aligned_and_lost_gap_isolated_without_packet_claim(self):
        text = (
            "#schema,rflink-sync-bits-v1\n#radio,868.30 MHz\n#mode,BENCH_ONLY\n"
            "#generation,8\n#blocks,3\n#capacity,256\n#full,0\n"
            "#recorded_bits,18\n"
            "index,epoch,bit_count,first_us,last_us,discontinuity_before,window_end,hex_msb\n"
            "0,10,3,100,150,0,0,A0\n"
            "1,10,9,175,300,0,1,A540\n"
            "2,11,6,400,530,1,0,FC\n"
        )
        meta, blocks = module.read_capture(self._csv(text))
        self.assertEqual(len(blocks), 3)
        self.assertEqual(blocks[0].bitstring(), "101")
        self.assertEqual(len(module.segments(blocks)), 2)
        self.assertEqual(module.segments(blocks)[0]["bit_count"], 12)
        self.assertEqual(module.segments(blocks)[1]["epoch"], 11)
        self.assertEqual(module.summarize(meta, blocks, module.segments(blocks))["ramses_messages_validated"], 0)

    def test_reject_tampered_padded_bit_and_count(self):
        text = (
            "#schema,rflink-sync-bits-v1\n#radio,868.30 MHz\n#mode,BENCH_ONLY\n"
            "#generation,1\n#blocks,1\n#capacity,256\n#full,0\n"
            "#recorded_bits,3\n"
            "index,epoch,bit_count,first_us,last_us,discontinuity_before,window_end,hex_msb\n"
            "0,1,3,1,3,0,0,A1\n"
        )
        with self.assertRaisesRegex(ValueError, "Nonzero unused low bits"):
            module.read_capture(self._csv(text))
        with self.assertRaises(ValueError):
            module.read_capture(self._csv(text.replace("#recorded_bits,3", "#recorded_bits,4")))
        with self.assertRaises(ValueError):
            module.read_capture(self._csv(text.replace("0,1,3,1,3", "1,1,3,1,3")))

    def test_cli_only_reports_structure_without_exposing_raw_bits(self):
        text = (
            "#schema,rflink-sync-bits-v1\n#radio,868.30 MHz\n#mode,BENCH_ONLY\n"
            "#generation,2\n#blocks,1\n#capacity,256\n#full,0\n"
            "#recorded_bits,8\n"
            "index,epoch,bit_count,first_us,last_us,discontinuity_before,window_end,hex_msb\n"
            "0,8,8,1,10,0,1,AB\n"
        )
        source=self._csv(text)
        out=Path(self.temp)/"run.json"
        run=subprocess.run([sys.executable, str(SCRIPT), str(source),
                            "--segments-json", str(out)],capture_output=True,text=True)
        self.assertEqual(run.returncode,0,run.stderr)
        self.assertTrue(out.is_file())
        self.assertIn('"hex_msb": "AB"',out.read_text())
        self.assertNotIn('"hex_msb"',run.stdout)
        self.assertIn('"rf_profile_validated": false',run.stdout)
        self.assertNotIn('AB',run.stdout)

    def test_guarded_endpoints_never_exist_in_default_web(self):
        web=WEB.read_text()
        main=(FW/"main/main.cpp").read_text()
        header=(HAL/"include/rflink_sync_868_bench_rx.hpp").read_text()
        code=(HAL/"rflink_sync_868_bench_rx.cpp").read_text()
        self.assertIn("static esp_err_t sync_bench_csv_get",web)
        self.assertIn("if(!key_ok(req))",web.split("sync_bench_csv_get",1)[1][:180])
        self.assertIn("if(!key_ok(req))",web.split("sync_bench_action_post",1)[1][:180])
        self.assertIn("if(!key_ok(req))",web.split("sync_bench_status_get",1)[1][:180])
        self.assertIn("#if defined(RFLINK_ENABLE_SYNC_868_BENCH) && RFLINK_ENABLE_SYNC_868_BENCH",web)
        self.assertIn("SyncBenchExportLease lease",web)
        self.assertIn("Cache-Control",web)
        self.assertIn("no-store",web)
        self.assertIn("#if RFLINK_ENABLE_SYNC_868_BENCH",main)
        self.assertIn("rflink_set_sync_868_bench_provider(rx868sync);",main)
        self.assertIn("kEvidenceBlocks = 256",header)
        self.assertIn("MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT",code)
        self.assertNotIn("ESP_LOGI(TAG,\"raw",code)
        self.assertIn("evidence_export_active_",header)


if __name__ == "__main__":
    unittest.main()
