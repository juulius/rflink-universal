"""Synthetic RFLink 433 signed pulse morphology diagnostics, read-only and private."""
from __future__ import annotations

import json
from pathlib import Path
import shutil
import subprocess
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/"tools"))
import rf_replay


def frame(scale=1):
    return [round(x*scale) for _ in range(12) for x in (350,-450,720,-450)]


def capture(radio, phase, seq, t, pulses):
    return rf_replay._parse_capture({
        "radio": radio, "frequency_hz": 433420000 if radio==0 else 433920000,
        "format": "signed_pulses_us", "capture_phase": phase,
        "boot_id":"2", "sequence":str(seq), "uptime_ms":t,
        "timestamp_utc_ms":1794671000000+t,
        "count":len(pulses), "pulses_us":pulses
    })


class WaveformMorphology(unittest.TestCase):
    def test_quality_indicators_do_not_change_decoder_verdict(self):
        base=frame()
        pre=capture(0,"pre_filter_rmt",100,1000,base+[125,-250])
        post=capture(0,"decoder_input",101,1000,base+[-5000])
        short=capture(0,"pre_filter_rmt",200,2000,[220,-420]*5)
        irregular=capture(0,"pre_filter_rmt",300,3000,[300,300,-420]*12)
        r=rf_replay.make_report([pre,post,short,irregular],{},[],structural_only=True)
        self.assertEqual(r["summary"]["radio_0"],4)
        self.assertEqual(r["rf433_insights"]["summary"]["paired_pre_post_filter"],1)
        wave=r["results"][1]["rf433_waveform"]
        self.assertTrue(wave["terminal_idle_candidate_removed"])
        self.assertEqual(wave["duration_count_without_idle_candidate"],48)
        self.assertEqual(wave["high_durations"],24)
        self.assertEqual(wave["low_durations"],24)
        self.assertEqual(wave["alternation_breaks"],0)
        self.assertEqual(wave["grouping_eligibility"],"eligible")
        self.assertEqual(wave["min_duration_us"],350)
        self.assertEqual(wave["max_duration_us"],720)
        self.assertIsInstance(wave["high_variation_cv"],float)
        self.assertEqual(r["results"][2]["rf433_waveform"]["grouping_eligibility"],
                         "too_few_durations")
        self.assertEqual(r["results"][3]["rf433_waveform"]["grouping_eligibility"],
                         "non_alternating_phases")
        per=r["rf433_insights"]["per_radio_waveform"]["radio_0"]
        self.assertEqual(per["physical_capture_candidates"],3)
        self.assertEqual(per["paired_pre_post_filter"],1)
        self.assertEqual(per["eligible"],1)
        self.assertEqual(per["too_few_durations"],1)
        self.assertEqual(per["non_alternating_phases"],1)
        self.assertEqual(per["count_1_to_15"],1)
        self.assertEqual(per["count_24_to_63"],2)
        self.assertEqual(r["rf433_insights"]["summary"]["eligible_for_grouping"],1)
        self.assertIn("rf433_waveform",r["results"][0])
        pair=r["results"][1]["rf433_evidence"]
        self.assertEqual(pair["pre_filter_median_duration_us"],
                         r["results"][0]["rf433_waveform"]["median_duration_us"])
        self.assertEqual(pair["decoder_input_median_duration_us"],wave["median_duration_us"])
        self.assertEqual(pair["pre_filter_short_durations"],0)
        self.assertEqual(pair["decoder_input_short_durations"],0)
        self.assertNotIn('"pulses_us":',json.dumps(r))
        self.assertNotIn('"_shape":',json.dumps(r))

    def test_43392_long_frame_and_868_unchanged(self):
        a=capture(1,"decoder_input",100,3000,frame(1.0))
        b=capture(1,"decoder_input",200,5000,frame(1.05))
        fifo=rf_replay._parse_capture({
            "radio":2,"frequency_hz":868299900,"format":"fifo_bytes_hex",
            "hex":"AB"*63,"count":63,"boot_id":"2","sequence":201
        })
        r=rf_replay.make_report([a,b,fifo],{},[],structural_only=True)
        per=r["rf433_insights"]["per_radio_waveform"]["radio_1"]
        self.assertEqual(per["physical_capture_candidates"],2)
        self.assertEqual(per["eligible"],2)
        self.assertEqual(per["count_24_to_63"],2)
        self.assertEqual(r["rf433_insights"]["summary"]["frames_in_repeated_groups"],2)
        self.assertEqual(r["summary"]["radio_2"],1)
        self.assertEqual(r["results"][2]["result"]["reason"],"native_decoder_not_run")
        self.assertNotIn("rf433_waveform",r["results"][2])

    @unittest.skipUnless(shutil.which("node"),"Node.js required")
    def test_compact_js_helpers_preserve_safe_waveform_without_raw_payloads(self):
        code=r"""
const fs=require('fs'),vm=require('vm'),assert=require('assert');
const src=fs.readFileSync('rflink_builder/static/replay_ui.js','utf8');
const begin=src.indexOf('function compactReplayDiagnosis(report){');
const end=src.indexOf('function compactReplayComparison(a,b){');
assert.ok(begin>=0&&end>begin);
const ctx={valid:r=>r?.schema==='rflink-offline-replay-v1'&&Array.isArray(r.results)};
vm.createContext(ctx);vm.runInContext(src.slice(begin,end),ctx);
const report={
 schema:'rflink-offline-replay-v1',capture_id_scheme:'stable-source-v2',capture_count:1,
 summary:{radio_0:1,radio_1:0,radio_2:0},
 rf433_insights:{version:1,summary:{physical_capture_candidates:1},
 per_radio_waveform:{radio_0:{
   physical_capture_candidates:1,eligible:1,frames_with_under_80us:1,
   count_24_to_63:1},radio_1:{physical_capture_candidates:0}}},
 results:[{capture_id:'a',radio:0,format:'signed_pulses_us',capture_phase:'decoder_input',
  count:48,pulses_us:[-222,444],secret:'SENSITIVE',result:{reason:'no_stable_timing_candidate',
   candidates:[{bits:'UNSAFE_BITS'}]},
  rf433_evidence:{paired_capture_id:'xyz',pre_filter_median_duration_us:325,
   decoder_input_median_duration_us:450,pre_filter_short_durations:2,
   decoder_input_short_durations:1},
  rf433_waveform:{grouping_eligibility:'eligible',high_durations:24,low_durations:24,
   alternation_breaks:0,median_duration_us:450,p90_duration_us:720,
   max_duration_us:720,durations_under_80us:1,durations_over_4000us:0,
   terminal_idle_candidate_removed:true,
   raw: 'LEAK_MARKER'}}]
};
const result=ctx.compactReplayDiagnosis(report);
assert.strictEqual(result.rf433_insights.per_radio_waveform.radio_0.eligible,1);
assert.strictEqual(result.rf433_insights.per_radio_waveform.radio_0.count_24_to_63,1);
const wave=result.longest_433_capture_examples.radio_0[0].waveform;
assert.strictEqual(wave.high_durations,24);
assert.strictEqual(wave.terminal_idle_candidate_removed,true);
const morph=result.longest_433_capture_examples.radio_0[0].pre_post_morphology;
assert.strictEqual(morph.pre_median_us,325);
assert.strictEqual(morph.post_median_us,450);
assert.strictEqual(morph.pre_under_80us,2);
assert.strictEqual(morph.post_under_80us,1);
for(const secret of ['UNSAFE_BITS','SENSITIVE','LEAK_MARKER','pulses_us'])
 assert.ok(!JSON.stringify(result).includes(secret),secret);
"""
        r=subprocess.run(["node","-e",code],cwd=ROOT,text=True,
                         capture_output=True,timeout=25)
        self.assertEqual(r.returncode,0,r.stderr)

    def test_mirrors_and_release_identity(self):
        self.assertEqual((ROOT/"tools/rf_replay.py").read_bytes(),
                         (ROOT/"rflink_builder/replay_tools/rf_replay.py").read_bytes())
        base=ROOT/"rflink_builder"
        self.assertIn("version: 0.6.47",(base/"config.yaml").read_text())
        self.assertIn("ARG BUILD_VERSION=0.6.47",(base/"Dockerfile").read_text())
        self.assertIn('class="version">v0.6.47',(base/"static/index.html").read_text())


if __name__=="__main__":
    unittest.main()
