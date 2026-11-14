"""Compact Replay diagnosis: no RAW RF values and bounded A/B snapshots."""
from pathlib import Path
import shutil
import subprocess
import unittest

ROOT=Path(__file__).resolve().parents[1]


class ReplayCompactExportTests(unittest.TestCase):
    def test_builder_has_one_click_compact_diagnosis_and_full_report_fallback(self):
        html=(ROOT/"rflink_builder/static/index.html").read_text()
        js=(ROOT/"rflink_builder/static/replay_ui.js").read_text()
        self.assertIn('id="rf-replay-copy-diagnosis"',html)
        self.assertIn('id="rf-replay-copy-report"',html)
        self.assertIn("function compactReplayDiagnosis(report)",js)
        self.assertIn("function compactReplayComparison(a,b)",js)
        self.assertIn("compactReplayDiagnosis(files.a)",js)
        self.assertIn("compactReplayDiagnosis(files.b)",js)
        self.assertIn("rf-replay-diagnosis-bundle-v1",js)
        self.assertNotIn("innerHTML",js)

    @unittest.skipUnless(shutil.which("node"),"Node.js required")
    def test_sanitization_and_a_b_merge_on_actual_js_helpers(self):
        test_program=r"""
const fs=require('fs'),vm=require('vm'),assert=require('assert');
const src=fs.readFileSync('rflink_builder/static/replay_ui.js','utf8');
const begin=src.indexOf('function compactReplayDiagnosis(report){');
const end=src.indexOf('function draw433Insights(slot){');
assert.ok(begin>=0 && end>begin);
const context={
 valid:r=>!!r&&r.schema==='rflink-offline-replay-v1'&&Array.isArray(r.results),
 index:r=>new Map((r?.results||[]).map(x=>[x.capture_id,x])),
 signature:x=>JSON.stringify(x?.result||{})
};
vm.createContext(context);
vm.runInContext(src.slice(begin,end),context);
const pulse=[350,-450,700,-450,350,-450,700,-450];
const a={
 schema:'rflink-offline-replay-v1',capture_id_scheme:'stable-source-v2',
 capture_count:3,decoder_mode:'native_firmware_codecs',decoder_fingerprint:'safe-codec',
 summary:{radio_0:0,radio_1:2,radio_2:1,
   insufficient_signed_pulses:2,legacy_preamble_absent:1},
 input_counters:{event_rows:6,events_without_rf_details:3},
 warnings:[],rf433_insights:{version:1,summary:{
   paired_pre_post_filter:1,physical_capture_candidates:1,
   repeated_groups:0},
   repeated_groups:[]},
 results:[
  {capture_id:'pre1',radio:1,format:'signed_pulses_us',capture_phase:'pre_filter_rmt',
   count:8,sequence:'100',boot_id:'2',uptime_ms:3000,
   pulses_us:pulse,secret:'private-RF',
   rf433_evidence:{paired_capture_id:'post1',pre_filter_durations:8,
    decoder_input_durations:7},
   result:{reason:'insufficient_signed_pulses',candidates:[{bits:'10110001001'}]}},
  {capture_id:'post1',radio:1,format:'signed_pulses_us',capture_phase:'decoder_input',
   count:7,sequence:'101',boot_id:'2',uptime_ms:3000,
   pulses_us:pulse,rf433_evidence:{paired_capture_id:'pre1',
   pre_filter_durations:8,decoder_input_durations:7},
   result:{reason:'no_stable_timing_candidate',candidates:[{bits:'SENSITIVE'}]}},
  {capture_id:'fifo1',radio:2,format:'fifo_bytes_hex',capture_phase:'decoder_input',
   count:63,hex:'FE'.repeat(63),result:{reason:'legacy_preamble_absent',
   candidates:[{bits:'DO-NOT-EXPOSE'}]}}
 ]};
const safe=context.compactReplayDiagnosis(a);
assert.strictEqual(safe.schema,'rflink-replay-diagnostic-summary-v1');
assert.strictEqual(safe.per_radio.radio_1,2);
assert.strictEqual(safe.rf433_insights.summary.paired_pre_post_filter,1);
assert.strictEqual(safe.longest_433_capture_examples.radio_1.length,1);
assert.strictEqual(safe.longest_433_capture_examples.radio_1[0].capture_id,'post1');
const str=JSON.stringify(safe);
for(const forbidden of ['SENSITIVE','10110001001','FEFEFE','private-RF',
 'pulses_us','fifo_bytes_hex'])assert.ok(!str.includes(forbidden),forbidden);
const b=JSON.parse(JSON.stringify(a));
b.results=b.results.filter(r=>r.capture_id!=='fifo1');
b.results[1].result.reason='candidate_timings_only';
const comparison=context.compactReplayComparison(a,b);
assert.strictEqual(comparison.compatible_capture_id_scheme,true);
assert.strictEqual(comparison.shared_capture_ids,2);
assert.strictEqual(comparison.changed_decoder_results,1);
assert.strictEqual(comparison.identical_decoder_results,1);
assert.strictEqual(comparison.only_a,1);
assert.strictEqual(comparison.only_b,0);
b.capture_id_scheme='legacy';
assert.strictEqual(context.compactReplayComparison(a,b).compatible_capture_id_scheme,false);
console.log('Compact RF diagnostics: safe bounded summaries and accurate A/B');
"""
        result=subprocess.run(["node","-e",test_program],cwd=ROOT,text=True,
                              capture_output=True,timeout=25)
        self.assertEqual(result.returncode,0,result.stderr)

    def test_release_version_matches_config(self):
        base=ROOT/"rflink_builder"
        self.assertIn("version: 0.6.46",(base/"config.yaml").read_text())
        self.assertIn("ARG BUILD_VERSION=0.6.46",(base/"Dockerfile").read_text())
        self.assertIn('class="version">v0.6.46',(base/"static/index.html").read_text())


if __name__=="__main__":
    unittest.main()
