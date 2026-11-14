"""RF Replay 433 v2 user interface, report and release regression checks."""
from pathlib import Path
import shutil
import subprocess
import sys
import unittest

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tools"))
import rf_replay


class Replay433UiContract(unittest.TestCase):
    def test_source_and_builder_decoder_are_identical(self):
        self.assertEqual((ROOT/"tools/rf_replay.py").read_bytes(),
                         (ROOT/"rflink_builder/replay_tools/rf_replay.py").read_bytes())

    def test_new_insights_not_breaking_existing_replay_schema(self):
        capture=rf_replay._parse_capture({
            "radio":1, "frequency_hz":433920000,
            "format":"signed_pulses_us", "pulses_us":[300,-500],
            "count":2, "capture_phase":"pre_filter_rmt"
        })
        report=rf_replay.make_report([capture], {}, [], structural_only=True)
        self.assertEqual(report["schema"],"rflink-offline-replay-v1")
        self.assertEqual(report["capture_count"],1)
        self.assertEqual(report["summary"]["radio_1"],1)
        self.assertIn("results",report)
        self.assertIn("rf433_insights",report)
        self.assertEqual(report["rf433_insights"]["summary"]["physical_capture_candidates"],1)

    def test_builder_replay_ui_has_group_drilldown_and_compatibility(self):
        html=(ROOT/"rflink_builder/static/index.html").read_text()
        js=(ROOT/"rflink_builder/static/replay_ui.js").read_text()
        css=(ROOT/"rflink_builder/static/app.css").read_text()
        for control in ("rf-replay-insights-a","rf-replay-insights-b",
                        "rf-replay-group-clear","rf-replay-results"):
            self.assertIn('id="'+control+'"',html)
        for term in ("draw433Insights('a')","draw433Insights('b')",
                     "selected433Group","duration_count_delta",
                     "rf433_evidence","group.group_id","function draw()"):
            self.assertIn(term,js)
        self.assertIn("Dit oudere Replay-rapport",js)
        self.assertIn("geen bewezen RF-verlies",js)
        self.assertIn(".rf-replay-insights-grid",css)
        self.assertNotIn("innerHTML",js)
        for existing in ("rf-replay-live-a","rf-replay-file-a","rf-replay-live-b"):
            self.assertIn('id="'+existing+'"',html)

    @unittest.skipUnless(shutil.which("node"),"Node.js required")
    def test_replay_ui_javascript_syntax(self):
        app=ROOT/"rflink_builder/static/replay_ui.js"
        result=subprocess.run(["node","--check",str(app)],
                              capture_output=True,text=True,timeout=20)
        self.assertEqual(result.returncode,0,result.stderr)

    def test_builder_release_identity(self):
        base=ROOT/"rflink_builder"
        self.assertIn("version: 0.6.44",(base/"config.yaml").read_text())
        self.assertIn("ARG BUILD_VERSION=0.6.44",(base/"Dockerfile").read_text())
        self.assertIn('class="version">v0.6.44',(base/"static/index.html").read_text())
        self.assertIn("## v0.6.44 ",(base/"CHANGELOG.md").read_text())


if __name__=="__main__":
    unittest.main()
