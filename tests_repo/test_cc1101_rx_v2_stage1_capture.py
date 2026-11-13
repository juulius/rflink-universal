"""First CC1101 RX Engine v2 milestone: honest 433 RMT counters and preserved RAW evidence."""
from pathlib import Path
import subprocess
import tempfile
import shutil
import unittest

ROOT=Path(__file__).resolve().parents[1]

def read(path):
    return (ROOT/path).read_text(encoding="utf-8")

class Rmt433EvidenceStage1(unittest.TestCase):
    def test_rmt_classifies_signed_durations_not_symbol_count(self):
        cpp=read("firmware/components/rflink_radio_hal/rflink_rmt_rx.cpp")
        header=read("firmware/components/rflink_radio_hal/include/rflink_rmt_rx.hpp")
        self.assertIn('if(n<2 || !complete)++stats_.short_frames;',cpp)
        self.assertNotIn('if(event.symbols<2 || !complete)++stats_.short_frames;',cpp)
        self.assertIn("stats_.last_raw_pulses=static_cast<uint32_t>(n);",cpp)
        self.assertIn("stats_.last_filtered_pulses=static_cast<uint32_t>(filtered);",cpp)
        self.assertIn('last_filtered_pulses',header)
        self.assertLess(cpp.index("capture_observer_(capture_context_"),cpp.index("filter_short_pulses_inplace("))
        self.assertLess(cpp.index("++stats_.forwarded_frames"),cpp.index("sink_->on_pulses"))

    def test_per_band_candidates_survive_868_noise_at_log_metadata_level(self):
        cpp=read("firmware/main/main.cpp")
        self.assertIn("const bool evidence_candidate=(stage==2 || count>=6);",cpp)
        self.assertIn("evidence_candidate?rflink::LogStream::signal:",cpp)
        self.assertIn("rflink::LogStream::noise;",cpp)
        self.assertIn("if(count<=2 && stage!=2)",cpp)
        self.assertIn("if(detail_logs && detail_logs->ready())",cpp)
        self.assertIn("detail_logs->save_rf(false",cpp)
        self.assertIn("kPreFilterKey|seq",cpp)
        self.assertIn('rflink::LogCode::rmt_raw_capture',cpp)

    @unittest.skipUnless(shutil.which('g++'), 'g++ required')
    def test_signal_metadata_retention_is_independent_of_noise_stream(self):
        source = r"""
#include "rflink_log_store.hpp"
#include <cassert>
using namespace rflink;
int main() {
  LogStore l;
  LogLimits config{};
  config.signal[1]=3;config.noise=2;
  assert(l.configure(config));
  const auto id=l.push(LogStream::signal,LogCode::rmt_raw_capture,1,1,
                       LogSeverity::info,30,0);
  assert(id>0);
  // Saturate the shared noise ring with unrelated 868 MHz noise.
  for(unsigned i=0;i<100;++i)
    l.push(LogStream::noise,LogCode::unknown_packet,i+2,2);
  LogRecord rows[16]{};
  LogQuery q{};q.radio=1;
  auto count=l.recent(rows,16,q);
  assert(count==1 && rows[0].sequence==id && rows[0].radio==1);
  // Earlier 433 noise still evicts from shared ring (intentional).
  q.stream=1;q.include_noise=true;
  assert(l.recent(rows,16,q)==0);
  // Follow-up milestone: RAW candidates have a NEW 433-only ring,
  // so they cannot evict genuine device signal events.
  const auto device=l.push(LogStream::signal,LogCode::user_rx_match,80,1);
  assert(device>id);
  for(unsigned i=0;i<260;++i)
    l.push(LogStream::signal,LogCode::rmt_raw_capture,i+110,1);
  const auto usage=l.stats();
  assert(usage.capture_used[1]==250);
  assert(usage.capture_overwritten[1]==11);
  assert(usage.used[1]==1 && usage.overwritten[1]==0);
  assert(usage.capture_used[0]==0);
  q.stream=0;q.include_noise=false;
  count=l.recent(rows,16,q);
  assert(count==16 && rows[count-1].radio==1);
  return 0;
}"""
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/'evidence.cpp';exe=Path(d)/'evidence'
            path.write_text(source)
            compiled=subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror',
                '-I'+str(ROOT/'firmware/main'),str(path),'-o',str(exe)],
                capture_output=True,text=True,timeout=30)
            self.assertEqual(compiled.returncode,0,compiled.stderr)
            ran=subprocess.run([str(exe)],capture_output=True,text=True,timeout=15)
            self.assertEqual(ran.returncode,0,ran.stderr)

    def test_api_and_native_builder_mirrors_explain_counters(self):
        web=read("firmware/main/rflink_web_ota.cpp")
        for radio in ("43342","43392"):
            self.assertIn('append_rmt_stages("'+radio+'"',web)
            # Dynamic channel suffixes are appended as JSON fields.
        for field in ("_rmt_non_saturated","_rmt_saturated",
                      "_raw_short_only","_last_raw_pulses","_last_filtered_pulses"):
            self.assertIn(field,web)
        for path in ("firmware/main/webui/app.js",
                     "rflink_builder/static/rflink_web/app.js"):
            app=read(path)
            self.assertIn("RMT buffers saturated (not decoded frames)",app)
            self.assertIn("decoder pulse inputs (not protocol matches)",app)
            self.assertIn("Long RAW candidates (6+ pulses or RMT saturation)",app)
            self.assertIn("Dedicated 433 MHz capture ring: does not evict recognized RF messages",app)
            self.assertIn("RMT capture completion does not prove a valid RF protocol message.",app)

if __name__=="__main__":
    unittest.main()
