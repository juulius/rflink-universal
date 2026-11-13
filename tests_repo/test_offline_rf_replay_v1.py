"""Offline replay regression tests use ONLY synthetic samples, never paired device captures."""
from __future__ import annotations

import csv
import json
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import rf_replay


def synthetic_pulses():
    # 16 signed high/low pairs; the +high width encodes 0/1, low stays fixed.
    return [value for i in range(16)
            for value in (320 if i % 2 == 0 else 720, -400)]


class OfflineReplayMilestone(unittest.TestCase):
    def test_433_pwm_candidate_does_not_claim_protocol(self):
        data = {"radio": 1, "seq": 42, "frequency_hz": 433920000,
                "count": 32, "format": "signed_pulses_us",
                "capture_phase": "pre_filter_rmt",
                "pulses_us": synthetic_pulses()}
        c = rf_replay._parse_capture(data, data)
        out = rf_replay.replay([c])
        result = out[0]["result"]
        self.assertIsNone(result["protocol"])
        self.assertEqual(result["reason"], "candidate_timings_only")
        pwm = [x for x in result["candidates"] if x["kind"] == "pulse_width_candidate"]
        self.assertEqual(len(pwm), 1)
        self.assertEqual(pwm[0]["bits"], "0101010101010101")
        self.assertFalse(pwm[0]["protocol_verified"])
        self.assertEqual(pwm[0]["confidence"], "timing_hypothesis_only")
        # Content ID includes capture identity, not just pulse equality.
        d2 = dict(data, seq=43)
        self.assertNotEqual(c["capture_id"],
                            rf_replay._parse_capture(d2, d2)["capture_id"])

    def test_tiny_433_gdo_noise_does_not_become_fake_bits(self):
        obj = {"radio": 0, "format": "signed_pulses_us",
               "frequency_hz": 433420000, "pulses_us": [275], "count": 1}
        report = rf_replay.make_report([rf_replay._parse_capture(obj, obj)], {}, [])
        verdict = report["results"][0]["result"]
        self.assertEqual(verdict["reason"], "insufficient_signed_pulses")
        self.assertEqual(verdict["candidates"], [])

    def test_rejects_mismatched_bands_invalid_bytes_and_length(self):
        base = {"radio": 2, "frequency_hz": 868299900,
                "format": "fifo_bytes_hex", "hex": "AA " * 63}
        with self.assertRaises(rf_replay.ReplayError):
            rf_replay._parse_capture(dict(base, radio=0))
        with self.assertRaises(rf_replay.ReplayError):
            rf_replay._parse_capture(dict(base, count=1))
        with self.assertRaises(rf_replay.ReplayError):
            rf_replay._parse_capture(dict(base, hex="AA " * 62))
        with self.assertRaises(rf_replay.ReplayError):
            rf_replay._parse_capture(dict(base, hex="GG " * 63))
        with self.assertRaises(rf_replay.ReplayError):
            rf_replay._parse_capture(dict(base, frequency_hz=433920000))
        with self.assertRaises(rf_replay.ReplayError):
            rf_replay._parse_capture(dict(base, hex="AA"*64))
        with self.assertRaises(rf_replay.ReplayError):
            rf_replay._parse_capture({"radio": 1, "format": "signed_pulses_us",
                                      "pulses_us": [0], "count": 1})

    def test_full_csv_ingest_missing_details_and_private_bundle_roundtrip(self):
        headers = ["record_type","sequence","uptime_ms","stream","radio",
                   "kind","code","count","detail_state","detail_type",
                   "detail_json","event_utc","boot_id"]
        pulse = {"format": "signed_pulses_us", "count": 32,
                 "frequency_hz": 433920000, "capture_phase": "pre_filter_rmt",
                 "pulses_us": synthetic_pulses(), "uptime_ms": 2200}
        fsk = {"format": "fifo_bytes_hex", "count": 63,
               "frequency_hz": 868299900, "capture_phase": "decoder_input",
               "hex": "A5 " * 63, "rssi_dbm": -91.5, "lqi": 17,
               "frequency_offset_hz": -300, "uptime_ms": 2400}
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp)/"RFLink_Diagnostics_Full.csv"
            with path.open("w", newline="", encoding="utf-8") as out:
                writer = csv.DictWriter(out, fieldnames=headers)
                writer.writeheader()
                writer.writerow({"record_type": "context", "kind": "device_status"})
                writer.writerow({"record_type": "event", "sequence": "1",
                                 "stream": "system", "code": "10"})
                writer.writerow({"record_type": "event", "sequence": "2",
                                 "radio": "1", "code": "46", "uptime_ms": "2200",
                                 "boot_id": "34", "detail_state": "retained",
                                 "detail_type": "signed_pulses_us",
                                 "detail_json": json.dumps(pulse)})
                writer.writerow({"record_type": "event", "sequence": "3",
                                 "radio": "2", "code": "2", "uptime_ms": "2400",
                                 "boot_id": "34", "detail_state": "retained",
                                 "detail_type": "fifo_bytes_hex",
                                 "detail_json": json.dumps(fsk)})
                writer.writerow({"record_type": "event", "sequence": "4",
                                 "radio": "2", "code": "2",
                                 "detail_state": "not_retained_or_overwritten"})
            captures, summary, warnings = rf_replay.load_captures(path)
            self.assertEqual(len(captures), 2)
            self.assertEqual(summary["missing_or_expired_rf_details"], 1)
            self.assertEqual(summary["events_without_rf_details"], 1)
            self.assertFalse(warnings)
            report = rf_replay.make_report(captures, summary, warnings, structural_only=True)
            self.assertEqual(report["summary"]["radio_2"], 1)
            self.assertEqual(report["summary"]["radio_1"], 1)
            self.assertEqual(report["summary"]["native_decoder_not_run"], 1)
            report_text = json.dumps(report)
            self.assertNotIn("A5A5A5A5", report_text)
            self.assertNotIn('"pulses_us":', report_text)
            self.assertEqual(report["decoder_mode"], "structural_only")
            outpath = Path(tmp)/"private_capture_bundle.json"
            outpath.write_text(json.dumps({"schema": rf_replay.BUNDLE_SCHEMA,
                                           "captures": captures}), encoding="utf-8")
            again, _, _ = rf_replay.load_captures(outpath)
            self.assertEqual(len(again), 2)
            self.assertEqual(again[0]["capture_id"], captures[0]["capture_id"])
            self.assertEqual(again[1]["quality"]["rssi_dbm"], -91.5)
            self.assertEqual(again[1]["hex"], captures[1]["hex"])

    @unittest.skipUnless(shutil.which("g++") or shutil.which("clang++"),
                         "native C++ compiler needed")
    def test_native_builtin_868_codecs_match_firmware_synthetic_frames(self):
        compiler = shutil.which("g++") or shutil.which("clang++")
        firmware = ROOT/"firmware/components/rflink_decoder_core"
        with tempfile.TemporaryDirectory() as tmp:
            t = Path(tmp)
            exe = rf_replay._compile_native(t)
            # This existing firmware test fixture has made-up addresses,
            # never actual RF captures, IDs, security keys or rolling codes.
            generator = t/"generator.cpp"
            generator.write_text(r"""
#include <cstdio>
#include "fixtures/synthetic_ramses.hpp"
int main(){
  const auto data=synthetic_ramses::frame(0x22F1);
  for(const auto b:data)std::printf("%02X",unsigned(b));
  std::printf("\n");
  return 0;
}
""", encoding="utf-8")
            built = subprocess.run([
                compiler, "-std=c++17","-O2",
                "-I"+str(firmware/"include"),
                "-I"+str(ROOT/"firmware/tests"),
                str(generator),str(firmware/"src/rflink_ramses_tx.cpp"),
                str(firmware/"src/rflink_ramses_codec.cpp"),
                "-o",str(t/"generator")],
                capture_output=True, text=True, timeout=45)
            self.assertEqual(built.returncode,0,built.stderr)
            packet = subprocess.check_output([str(t/"generator")],text=True).strip()
            self.assertEqual(len(packet),126)
            # True checksum/Manchester/UART decode, and two deliberately
            # invalid 63-byte packets. Never infer CRC-OK from LQI.
            stream = packet + "\n" + ("FF"*63) + "\n" + ("00B32AAB2A"+"00"*58) + "\n"
            run = subprocess.run([str(exe)],input=stream,text=True,
                                 capture_output=True,timeout=20)
            self.assertEqual(run.returncode,0,run.stderr)
            answers=[json.loads(line) for line in run.stdout.splitlines()]
            self.assertEqual(len(answers),3)
            self.assertEqual(answers[0]["protocol"],"ramses_ii")
            self.assertEqual(answers[0]["command"],0x22F1)
            self.assertFalse(answers[0]["hardware_crc_verified"])
            self.assertEqual(answers[1]["reason"],"legacy_preamble_absent")
            self.assertEqual(answers[2]["reason"],"built_in_decoder_rejected")

    def test_replay_never_touches_radio_or_embeds_user_fixtures(self):
        helper = (ROOT/"tools/rf_replay_codecs.cpp").read_text(encoding="utf-8")
        engine = (ROOT/"tools/rf_replay.py").read_text(encoding="utf-8")
        self.assertIn("rflink_ramses_ii::decodePacked", helper)
        self.assertIn("rflink::itho::decode_packet", helper)
        self.assertIn("hardware_crc_verified", helper)
        self.assertNotIn("esp_wifi", helper)
        self.assertNotIn("rf_transmit", helper)
        self.assertNotIn("requests.", engine)
        self.assertIn("MAX_CSV_BYTES", engine)
        self.assertIn("BUNDLE_SCHEMA", engine)


if __name__ == "__main__":
    unittest.main()
