"""868 synchronous bench controls belong to Logs but must stay isolated.

These tests do not connect to hardware, a management key or actual RF frames.
"""
from pathlib import Path
import json
import shutil
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[1]
HTML = (ROOT / "firmware/main/webui/index.html").read_text(encoding="utf-8")
JS = (ROOT / "firmware/main/webui/app.js").read_text(encoding="utf-8")
FW = (ROOT / "firmware/main/rflink_web_ota.cpp").read_text(encoding="utf-8")


class Sync868LogViewerContract(unittest.TestCase):
    def test_capture_panel_inside_existing_logs_not_a_new_page(self):
        before, after = HTML.split('data-view="logs"', 1)
        self.assertIn('id="sync868-panel"', after.split('data-view="learn"', 1)[0])
        for action in ("status", "arm", "freeze", "export", "feedback", "analysis"):
            self.assertIn(f'id="sync868-{action}"', after)
        self.assertIn('id="sync868-key" type="password" autocomplete="off"', after)
        self.assertIn("separate psram", after.lower())

    def test_capture_api_is_opt_in_and_still_requires_the_key(self):
        self.assertIn("const sync868Path='/api/diagnostics/868-sync'", JS)
        self.assertIn("'X-RFLink-Update-Key':key", JS)
        self.assertIn("sync868TrustedLan()", JS)
        self.assertIn("window.confirm(", JS)
        self.assertIn("if($('log-radio').value==='2')", JS)
        self.assertIn("sync868Command('arm')", JS)
        self.assertIn("sync868Command('freeze')", JS)
        self.assertIn("fetch(sync868Path+'.csv'", JS)
        self.assertNotIn("localStorage", JS[JS.index("// Experimental 868 synchronous capture"):
                                             JS.index("// The server streams CSV")])
        for func in ("sync_bench_status_get", "sync_bench_action_post",
                     "sync_bench_csv_get"):
            snippet = FW.split("static esp_err_t " + func, 1)[1][:200]
            self.assertIn("if(!key_ok(req))", snippet)

    def test_raw_data_stays_out_of_app_log_stream(self):
        capture = JS.split("// Experimental 868 synchronous capture", 1)[1].split(
            "// The server streams CSV", 1)[0]
        self.assertNotIn("auditUi(", capture)
        self.assertNotIn("logRows", capture)
        self.assertIn("inspectSync868Csv(await blob.text())", capture)
        self.assertIn("URL.createObjectURL(blob)", capture)
        self.assertIn("512*1024", capture)
        self.assertIn("Not a protocol decode", capture)

    @unittest.skipUnless(shutil.which("node"), "Node.js unavailable")
    def test_browser_inspector_validates_counts_and_epoch_boundaries(self):
        # Evaluate ONLY the pure inspector function in a Node vm, no DOM/network.
        source = JS.split("function inspectSync868Csv(csv){", 1)[1].split(
            "async function exportSync868()", 1)[0]
        code = "function inspectSync868Csv(csv){" + source
        fixture = (
            "#schema,rflink-sync-bits-v1\n#radio,868.30 MHz\n#mode,BENCH_ONLY\n"
            "#generation,8\n#blocks,3\n#capacity,256\n#full,0\n"
            "#recorded_bits,18\n"
            "index,epoch,bit_count,first_us,last_us,discontinuity_before,window_end,hex_msb\n"
            "0,10,3,100,150,0,0,A0\n"
            "1,10,9,175,300,0,1,A580\n"
            "2,11,6,400,530,1,0,FC\n"
        )
        runner = """
const vm=require('node:vm');
const source=process.argv[1],fixture=process.argv[2];
const sandbox={};vm.createContext(sandbox);
vm.runInContext(source,sandbox);
const good=sandbox.inspectSync868Csv(fixture);
if(good.blocks!==3 || good.bits!==18 || good.segments!==2 ||
   good.epochs!==2 || good.discontinuities!==1)
  throw new Error('Incorrect loss-aware summary '+JSON.stringify(good));
if(!(good.rate>49000 && good.rate<49300))
  throw new Error('Incorrect measured effective rate');
for(const bad of [
  fixture.replace('0,10,3,100,150,0,0,A0','0,10,3,100,150,0,0,A1'),
  fixture.replace('#recorded_bits,18','#recorded_bits,19'),
  fixture.replace('1,10,9,175,300','3,10,9,175,300'),
  fixture.replace('#blocks,3','#blocks,4')
]){
  let rejected=false;
  try{sandbox.inspectSync868Csv(bad);}catch{rejected=true;}
  if(!rejected)throw new Error('Malformed capture was accepted');
}
console.log(JSON.stringify(good));
"""
        run = subprocess.run(
            ["node", "-e", runner, code, fixture],
            capture_output=True, text=True, timeout=15
        )
        self.assertEqual(run.returncode, 0, run.stderr)
        result = json.loads(run.stdout)
        self.assertEqual(result["segments"], 2)


if __name__ == "__main__":
    unittest.main()
