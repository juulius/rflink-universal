"""Stable Replay capture identity across UTC recalculations, without false repeat labels."""
from __future__ import annotations
import csv
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/"tools"))
import rf_replay


def pulses():
    # Fictional OOK waveform, never obtained from a private RF receiver.
    return [x for _ in range(12) for x in (320,-400,640,-400)]


def event_row(sequence, utc, phase="pre_filter_rmt", *, radio=1, boot="2", data=None):
    return {
        "record_type": "event", "detail_state": "retained",
        "detail_type": "signed_pulses_us", "code": "46" if phase=="pre_filter_rmt" else "1",
        "radio": str(radio), "boot_id": boot, "sequence": str(sequence),
        "uptime_ms": "860", "event_utc": utc,
        "detail_json": json.dumps({
            "radio": radio, "frequency_hz": 433920000 if radio==1 else 433420000,
            "format": "signed_pulses_us", "capture_phase": phase,
            "pulses_us": pulses() if data is None else data,
            "count": len(pulses() if data is None else data),
            "uptime_ms": 860,
            # Time derived by the device at export, may vary on later export.
            "timestamp_utc_ms": 1794671062659 if utc.endswith("659Z") else 1794671062629,
        })
    }


def from_csv(rows):
    fields = ["record_type", "detail_state", "detail_type", "code",
              "radio", "boot_id", "sequence", "uptime_ms", "event_utc", "detail_json"]
    buf = io.StringIO(newline="")
    writer = csv.DictWriter(buf, fieldnames=fields)
    writer.writeheader()
    writer.writerows(rows)
    with tempfile.TemporaryDirectory() as tmp:
        filename=Path(tmp)/"RFLink_Diagnostics_Full.csv"
        filename.write_text(buf.getvalue(), encoding="utf-8")
        captures, counters, warnings = rf_replay.load_captures(filename)
    return rf_replay.make_report(captures, counters, warnings, structural_only=True)


class StableCaptureIds(unittest.TestCase):
    def test_identical_event_has_same_id_after_utc_drift(self):
        a=from_csv([event_row(8,"2026-11-14T15:44:22.659Z")])
        b=from_csv([event_row(8,"2026-11-14T15:44:22.629Z")])
        self.assertEqual(a["capture_id_scheme"], "stable-source-v2")
        self.assertEqual(a["results"][0]["capture_id"], b["results"][0]["capture_id"])
        self.assertNotEqual(a["results"][0]["timestamp_utc_ms"],
                            b["results"][0]["timestamp_utc_ms"])
        self.assertNotEqual(a["results"][0]["event_utc"],b["results"][0]["event_utc"])

    def test_no_collision_on_new_boot_seq_radio_content_or_phase(self):
        original=event_row(100,"2026-11-14T15:44:22.659Z")
        captures=[
            from_csv([original])["results"][0]["capture_id"],
            from_csv([dict(original,sequence="101")])["results"][0]["capture_id"],
            from_csv([dict(original,boot_id="3")])["results"][0]["capture_id"],
            from_csv([event_row(100,"2026-11-14T15:44:22.659Z",radio=0)])["results"][0]["capture_id"],
            from_csv([event_row(100,"2026-11-14T15:44:22.659Z",data=pulses()[:-2])])["results"][0]["capture_id"],
            from_csv([event_row(100,"2026-11-14T15:44:22.659Z",phase="decoder_input")])["results"][0]["capture_id"],
        ]
        self.assertEqual(len(set(captures)),len(captures))

    def test_pair_is_not_mislabelled_as_repeated_transmission(self):
        rows=[
            event_row(302,"2026-11-14T15:44:22.659Z"),
            event_row(303,"2026-11-14T15:44:22.659Z","decoder_input")
        ]
        r=from_csv(rows)
        info=r["rf433_insights"]
        self.assertEqual(info["summary"]["paired_pre_post_filter"],1)
        self.assertEqual(info["summary"]["physical_capture_candidates"],1)
        self.assertEqual(info["summary"]["repeated_groups"],0)
        for record in r["results"]:
            evidence=record["rf433_evidence"]
            self.assertIn("paired_capture_id",evidence)
            self.assertNotIn("group_id",evidence)
            self.assertNotIn("group_count",evidence)

    def test_two_separate_frames_keep_repeated_group_and_size(self):
        rows=[
            event_row(302,"2026-11-14T15:44:22.659Z"),
            event_row(303,"2026-11-14T15:44:22.659Z","decoder_input"),
            event_row(402,"2026-11-14T15:44:24.659Z"),
            event_row(403,"2026-11-14T15:44:24.659Z","decoder_input"),
        ]
        r=from_csv(rows)
        self.assertEqual(r["rf433_insights"]["summary"]["repeated_groups"],1)
        self.assertEqual(r["rf433_insights"]["summary"]["frames_in_repeated_groups"],2)
        self.assertEqual(len(r["rf433_insights"]["repeated_groups"]),1)
        for row in r["results"]:
            ev=row["rf433_evidence"]
            self.assertEqual(ev["group_count"],2)
            self.assertEqual(ev["group_id"],"G1")

    def test_868_payload_identity_does_not_depend_on_utc(self):
        raw={"radio":2,"frequency_hz":868299900,"format":"fifo_bytes_hex",
             "hex":"AA"*63,"count":63,"boot_id":"2","sequence":"100",
             "uptime_ms":2200,"event_utc":"2026-11-14T15:00:00.000Z"}
        a=rf_replay._parse_capture(raw)
        b=rf_replay._parse_capture({**raw,
              "event_utc":"2026-11-14T15:00:00.030Z",
              "timestamp_utc_ms":1794671062659})
        self.assertEqual(a["capture_id"],b["capture_id"])

    def test_decoder_is_exactly_mirrored_for_builder_and_cli(self):
        self.assertEqual((ROOT/"tools/rf_replay.py").read_bytes(),
                         (ROOT/"rflink_builder/replay_tools/rf_replay.py").read_bytes())

    def test_ui_explicitly_warns_when_comparing_legacy_reports(self):
        source=(ROOT/"rflink_builder/static/replay_ui.js").read_text()
        self.assertIn("capture_id_scheme!=='stable-source-v2'",source)
        self.assertIn("oud Replay-rapport",source)
        self.assertIn("evidence?.group_count>=2",source)


if __name__=="__main__":
    unittest.main()
