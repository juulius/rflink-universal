"""Replay isolated automatic bit windows without fabricating continuity."""
import csv
import json
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tools"))
import rf_replay


def window(seq=1,hex_data="AA"*4+"00"*59):
    return {"radio":2,"frequency_hz":868300000,"format":"clocked_bits_hex",
            "capture_phase":"sync868_preamble_window","count":63,"bit_count":504,
            "boot_id":"4","sequence":seq,"uptime_ms":100+seq,
            "hex":hex_data,"carrier_context":{"state":"sampled","samples":1,
                "carrier_samples":1,"min_rssi_dbm_x2":-160,"max_rssi_dbm_x2":-160,
                "extra_secret":"DO_NOT_COPY"}}


class OfflineSyncReplay(unittest.TestCase):
    def test_independent_groups_and_private_roundtrip(self):
        with tempfile.TemporaryDirectory() as folder:
            path=Path(folder)/"a.json"
            # Same capture twice is not a second RF event. One changed bit
            # must not enter the exact-repeat group.
            items=[window(),window(),window(2),window(3,"AA"*4+"00"*58+"01")]
            items.append({"schema":"rflink-rf-capture-v1","type":"sync868_preamble_window",
                          "capture":{**window(4),"hex":"55"*63}})
            path.write_text(json.dumps(items))
            caps,counts,warnings=rf_replay.load_captures(path)
            self.assertEqual(len(caps),5)
            self.assertFalse(warnings)
            report=rf_replay.make_report(caps,counts,warnings,structural_only=True)
            stats=report["rf868_insights"]["summary"]
            self.assertEqual(stats["windows"],5)
            self.assertEqual(stats["exact_repeat_groups"],1)
            self.assertEqual(stats["windows_in_repeat_groups"],2)
            self.assertEqual(stats["windows_with_carrier_high"],5)
            for result in report["results"][:3]:
                self.assertEqual(result["rf868_evidence"]["group_count"],2)
                self.assertEqual(result["rf868_evidence"]["alternating_prefix"],32)
                self.assertEqual(result["rf868_evidence"]["tail_ones"],0)
            self.assertNotIn("group_id",report["results"][3]["rf868_evidence"])
            self.assertNotIn("group_id",report["results"][4]["rf868_evidence"])
            self.assertNotIn("DO_NOT_COPY",json.dumps(report))
            self.assertNotIn('"hex":',json.dumps(report))
            path.write_text(json.dumps({"schema":rf_replay.BUNDLE_SCHEMA,"captures":caps}))
            again,_,issues=rf_replay.load_captures(path)
            self.assertFalse(issues)
            self.assertEqual([c["capture_id"] for c in caps],[c["capture_id"] for c in again])
            self.assertEqual(again[0]["carrier_context"],caps[0]["carrier_context"])

    def test_csv_and_invalid_windows(self):
        for patch in ({"radio":1},{"bit_count":503},{"hex":"AA"*62},{"count":504}):
            with self.assertRaises(rf_replay.ReplayError):
                rf_replay._parse_capture({**window(),**patch})
        with tempfile.TemporaryDirectory() as folder:
            path=Path(folder)/"a.csv"
            fields=["record_type","detail_state","detail_type","detail_json","radio","code",
                    "sequence","boot_id","uptime_ms"]
            with path.open("w",newline="") as stream:
                writer=csv.DictWriter(stream,fieldnames=fields);writer.writeheader()
                writer.writerow({"record_type":"event","radio":2,"code":55,"sequence":1,
                    "boot_id":4,"uptime_ms":101,"detail_state":"retained",
                    "detail_type":"clocked_bits_hex","detail_json":json.dumps(window())})
                writer.writerow({"record_type":"event","radio":2,"code":55,"sequence":2,
                    "detail_state":"not_retained_or_overwritten"})
                writer.writerow({"record_type":"event","radio":1,"code":46,"sequence":3,
                    "detail_state":"retained","detail_type":"signed_pulses_us",
                    "detail_json":json.dumps({"radio":1,"format":"signed_pulses_us",
                        "capture_phase":"pre_filter_rmt","pulses_us":[320,-640]*16,
                        "decoder_input_count":31})})
            caps,counters,issues=rf_replay.load_captures(path)
            self.assertFalse(issues)
            self.assertEqual(counters["missing_or_expired_rf_details"],1)
            report=rf_replay.make_report(caps,counters,issues,structural_only=True)
            self.assertEqual(report["capture_count"],2)
            self.assertEqual(report["results"][1]["rf433_evidence"]["pairing"],
                             "consolidated_metadata_only")
            self.assertEqual(report["results"][1]["rf433_evidence"]["decoder_input_durations"],31)
            self.assertEqual(report["rf433_insights"]["summary"]["physical_capture_candidates"],1)

    @unittest.skipUnless(shutil.which("g++") or shutil.which("clang++"),"native compiler required")
    def test_native_scanner_distinguishes_sync_and_fifo(self):
        with tempfile.TemporaryDirectory() as folder:
            binary=rf_replay._compile_native(Path(folder))
            run=subprocess.run([str(binary)],input="SYNC:"+"00"*63+"\n"+"00"*63+"\n",
                               capture_output=True,text=True,timeout=20)
            self.assertEqual(run.returncode,0,run.stderr)
            a,b=map(json.loads,run.stdout.splitlines())
            self.assertEqual(a["reason"],"sync_no_match_in_window")
            self.assertEqual(a["sync_scan"]["ramses_checksum_valid"],0)
            self.assertEqual(b["reason"],"legacy_preamble_absent")
            self.assertFalse(a["hardware_crc_verified"])

    def test_mirrored_tools_and_ui(self):
        for name in ("rf_replay.py","rf_replay_codecs.cpp"):
            self.assertEqual((ROOT/"tools"/name).read_bytes(),
                             (ROOT/"rflink_builder/replay_tools"/name).read_bytes())
        js=(ROOT/"rflink_builder/static/replay_ui.js").read_text()
        self.assertIn("report.rf868_insights",js)
        self.assertIn("sync_scan:r.sync_scan??null",js)
        self.assertIn("not verified repeat transmissions",js)


if __name__=="__main__":
    unittest.main()
