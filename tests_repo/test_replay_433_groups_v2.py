"""433 offline replay evidence: strict pre/post linking and conservative repetition groups."""
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import rf_replay


def example_frame(scale=1.0):
    # A fabricated, alternating OOK waveform. Not an actual user capture.
    return [round(v * scale) for _ in range(12)
            for v in (320, -400, 640, -400)]


def capture(radio, phase, seq, ms, pulses, boot="2"):
    raw = {
        "radio": radio,
        "frequency_hz": (433420000 if radio == 0 else 433920000),
        "format": "signed_pulses_us",
        "capture_phase": phase,
        "boot_id": boot,
        "sequence": str(seq),
        "uptime_ms": ms,
        "timestamp_utc_ms": 1794671060000 + ms,
        "event_utc": "2026-11-14T15:44:22Z",
        "count": len(pulses),
        "pulses_us": pulses,
    }
    return rf_replay._parse_capture(raw)


def report(data):
    return rf_replay.make_report(data, {}, [], structural_only=True)


class Replay433Groups(unittest.TestCase):
    def test_pre_post_count_delta_and_repeat_groups_count_once(self):
        frame = example_frame()
        captures = [
            capture(1, "pre_filter_rmt", 100, 1000, frame),
            capture(1, "decoder_input", 101, 1000, frame + [-5000]),
            capture(1, "pre_filter_rmt", 200, 2000, example_frame(1.05)),
            capture(1, "decoder_input", 201, 2000, example_frame(1.05) + [-5000]),
            capture(1, "pre_filter_rmt", 210, 2020, [115]),
        ]
        result = report(captures)
        insights = result["rf433_insights"]
        s = insights["summary"]
        self.assertEqual(s["paired_pre_post_filter"], 2)
        self.assertEqual(s["pairs_with_duration_count_change"], 2)
        self.assertEqual(s["physical_capture_candidates"], 3)
        self.assertEqual(s["eligible_for_grouping"], 2)
        self.assertEqual(s["too_short_or_irregular_for_grouping"], 1)
        self.assertEqual(s["repeated_groups"], 1)
        self.assertEqual(s["frames_in_repeated_groups"], 2)
        self.assertEqual(len(insights["repeated_groups"]), 1)
        group = insights["repeated_groups"][0]
        self.assertEqual(group["radio"], 1)
        self.assertEqual(group["count"], 2)
        self.assertFalse(group["protocol_verified"])
        self.assertEqual(len(group["sample_capture_ids"]), 2)
        rows = result["results"]
        for i in range(4):
            self.assertEqual(rows[i]["rf433_evidence"]["group_id"], group["group_id"])
            self.assertEqual(rows[i]["rf433_evidence"]["duration_count_delta"], 1)
        self.assertEqual(rows[0]["rf433_evidence"]["paired_capture_id"],
                         rows[1]["capture_id"])
        self.assertEqual(rows[1]["rf433_evidence"]["paired_capture_id"],
                         rows[0]["capture_id"])
        self.assertNotIn("rf433_evidence", rows[-1])
        encoded = json.dumps(result)
        self.assertNotIn('"pulses_us"', encoded)
        self.assertNotIn('"hex"', encoded)
        self.assertNotIn('"_shape"', encoded)

    def test_pairing_rejects_other_boot_wrong_clock_and_sequence_gaps(self):
        wave = example_frame()
        candidates = [
            (capture(0, "pre_filter_rmt", 1, 500, wave, "1"),
             capture(0, "decoder_input", 2, 500, wave, "2")),
            (capture(0, "pre_filter_rmt", 1, 500, wave),
             capture(0, "decoder_input", 2, 501, wave)),
            (capture(0, "pre_filter_rmt", 1, 500, wave),
             capture(0, "decoder_input", 4, 500, wave)),
            (capture(0, "pre_filter_rmt", 1, 500, wave),
             capture(1, "decoder_input", 2, 500, wave)),
        ]
        for first, second in candidates:
            with self.subTest(boot=second["boot_id"], seq=second["sequence"],
                              ms=second["uptime_ms"], radio=second["radio"]):
                s = report([first, second])["rf433_insights"]["summary"]
                self.assertEqual(s.get("paired_pre_post_filter", 0), 0)

    def test_433_band_isolation_and_wrong_shapes_never_group(self):
        waveform = example_frame()
        corrupt = [v * (3 if i % 4 == 0 else 1)
                   for i, v in enumerate(waveform)]
        result = report([
            capture(0, "pre_filter_rmt", 1, 500, waveform),
            capture(1, "pre_filter_rmt", 2, 500, waveform),
            capture(1, "pre_filter_rmt", 3, 1500, corrupt),
            capture(1, "pre_filter_rmt", 4, 2500, [-x for x in waveform]),
        ])
        summary = result["rf433_insights"]["summary"]
        self.assertEqual(summary["repeated_groups"], 0)
        self.assertEqual(summary["groups_found_including_singletons"], 4)

    def test_868_fifo_does_not_enter_433_analysis(self):
        packet = rf_replay._parse_capture({
            "radio": 2, "frequency_hz": 868299900, "format": "fifo_bytes_hex",
            "count": 63, "hex": "AA" * 63
        })
        result = report([packet])
        self.assertEqual(result["rf433_insights"]["summary"].get(
            "physical_capture_candidates", 0), 0)
        self.assertNotIn("rf433_evidence", result["results"][0])
        self.assertEqual(result["summary"]["radio_2"], 1)

    def test_builder_and_cli_use_identical_decoder(self):
        a = (ROOT / "tools/rf_replay.py").read_bytes()
        b = (ROOT / "rflink_builder/replay_tools/rf_replay.py").read_bytes()
        self.assertEqual(a, b)


if __name__ == "__main__":
    unittest.main()
