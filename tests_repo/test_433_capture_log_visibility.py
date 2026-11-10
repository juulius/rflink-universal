"""433 MHz RMT-to-LogStore observability regressions.

RMT counters are capture counts, not decoded or retained log counts.
"""
import pathlib
import shutil
import subprocess
import tempfile
import unittest

ROOT=pathlib.Path(__file__).resolve().parents[1]
HAL=ROOT/'firmware/components/rflink_radio_hal'
WEB=(ROOT/'firmware/main/rflink_web_ota.cpp').read_text()
APP=(ROOT/'firmware/main/webui/app.js').read_text()
HTML=(ROOT/'firmware/main/webui/index.html').read_text()


class RadioCaptureVisibility(unittest.TestCase):
    def test_precise_receive_stage_counters(self):
        header=(HAL/'include/rflink_rmt_rx.hpp').read_text()
        source=(HAL/'rflink_rmt_rx.cpp').read_text()
        for field in ('full_frames','filtered_frames','forwarded_frames','short_frames'):
            with self.subTest(field=field):
                self.assertIn(field,header)
                self.assertIn('stats_.'+field,source)
                self.assertIn('rx43342_'+field,WEB)
                self.assertIn('rx43392_'+field,WEB)
        self.assertIn('if(filtered>=2)',source)
        self.assertIn('sink_->on_pulses',source)
        self.assertLess(source.index('++stats_.forwarded_frames'),source.index('sink_->on_pulses'))
        self.assertIn('event.symbols<kMaxSymbols',source)
        self.assertIn('const size_t filtered=filter_short_pulses_inplace(',source)

    def test_log_ui_explains_raw_capture_loss(self):
        for label in ('RMT captures','Inspect RX logs','log-capture-diagnostics'):
            self.assertIn(label,HTML)
        for label in ('renderLogCaptureDiagnostics','forwarded_frames',
                      'full_frames','filtered_frames','decode_pulses_',
                      "selected==='0'?'43342':'43392'",'latest 48 matching records',
                      "button.dataset.radioLog", "$('log-noise').checked=true"):
            with self.subTest(label=label):
                self.assertIn(label,APP)
        self.assertIn("if((logSource==='radio'||logSource==='all')&&radio)",APP)
        self.assertIn("p.set('radio',radio)",APP)
        self.assertIn("if(noise&&['all','radio'].includes(logSource))p.set('noise','1')",APP)

    @unittest.skipUnless(shutil.which('g++'), 'g++ required')
    def test_noise_ring_can_evict_old_433_records_despite_scoped_query(self):
        code=r'''
#include "rflink_log_store.hpp"
#include <cassert>
using namespace rflink;
int main(){
  LogStore logs;
  LogLimits limits{};
  limits.noise=4;
  assert(logs.configure(limits));
  logs.push(LogStream::noise,LogCode::raw_pulses,10,1);
  LogRecord records[48]{};
  LogQuery q{};q.stream=1;q.radio=1;q.include_noise=true;
  assert(logs.recent(records,48,q)==1);
  for(unsigned i=0;i<6;++i)
    logs.push(LogStream::noise,LogCode::unknown_packet,20+i,2);
  // Selecting a frequency cannot restore events overwritten by the shared ring.
  assert(logs.recent(records,48,q)==0);
  logs.push(LogStream::noise,LogCode::raw_pulses,50,1);
  assert(logs.recent(records,48,q)==1);
  assert(records[0].radio==1);
  return 0;
}
'''
        with tempfile.TemporaryDirectory() as d:
            src=pathlib.Path(d)/'test.cpp';exe=pathlib.Path(d)/'test'
            src.write_text(code)
            compile=subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror',
                                    '-I'+str(ROOT/'firmware/main'),str(src),'-o',str(exe)],
                                   capture_output=True,text=True,timeout=30)
            self.assertEqual(compile.returncode,0,compile.stderr)
            run=subprocess.run([str(exe)],capture_output=True,text=True,timeout=15)
            self.assertEqual(run.returncode,0,run.stderr)


if __name__=='__main__':
    unittest.main()
