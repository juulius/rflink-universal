"""Regression for post-decoder 433 classification and bounded original evidence.

This also checks that ordinary noise cannot evict the retained band evidence.
"""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
MAIN = ROOT / "firmware/main"

CPP = r'''
#include "rflink_log_store.hpp"
#include "rflink_log_details.hpp"
#include <cassert>
#include <cstring>
#include <memory>
using namespace rflink;
int main(){
  using D=Rx433Disposition;
  const auto unknown=LogCode::raw_pulses;
  assert(classify_433_capture(1,1,0,false,unknown)==D::noise);
  assert(classify_433_capture(2,1024,0,false,unknown)==D::noise);
  assert(classify_433_capture(0,12,0,false,unknown)==D::noise);
  assert(classify_433_capture(0,12,3,true,unknown)==D::noise);
  assert(classify_433_capture(0,49,50,true,unknown)==D::raw_candidate);
  assert(classify_433_capture(0,4,5,true,LogCode::user_rx_match)==D::recognized);
  assert(classify_433_capture(0,49,50,true,LogCode::acurite_609txc)==D::recognized);
  assert(classify_433_capture(0,49,50,true,LogCode::fineoffset_wh2)==D::recognized);
  assert(classify_433_capture(0,4,5,true,LogCode::user_rx_ambiguous)==D::raw_candidate);

  auto log=std::make_unique<LogStore>();
  auto detail=std::make_unique<DetailStore>();
  assert(log->configure({}) && detail->configure());
  assert(kRaw433CaptureRetention==500 && kUnknown868Retention==500);
  assert(DetailStore::kEntryLimits[0]==750 && DetailStore::kEntryLimits[1]==750);
  static_assert(sizeof(DetailStore)+2368*1024+1024*sizeof(int32_t)
                <=DetailStore::kMaxBudgetBytes,"staging fits detail budget");
  constexpr uint64_t keybase=(uint64_t(1)<<63)|(uint64_t(1)<<62);
  int32_t original[49]{};
  for(unsigned i=0;i<49;++i)original[i]=(i&1)?-640:320;
  const auto message=log->push(LogStream::signal,LogCode::user_rx_match,
      1,1,LogSeverity::info,49,0,100,kUnified433Capture,25,255,0,0,50);
  assert(message && detail->save_rf(false,1,keybase|message,1,
      433920000,original,nullptr,49));
  for(unsigned i=0;i<500;++i){
    const auto seq=log->push(LogStream::signal,LogCode::rmt_raw_capture,
        i+2,1,LogSeverity::info,49,0,i+101,kUnified433Capture,25,255,0,0,50);
    assert(seq && detail->save_rf(false,1,keybase|seq,i+2,
        433920000,original,nullptr,49));
  }
  const auto before=detail->stats();
  assert(before.entries[1]==501 && before.evicted[1]==0);
  // Sampled rejected/noisy waveforms consume only shared noise storage.
  int32_t glitch=25;
  for(unsigned i=0;i<2100;++i){
    const auto seq=log->push(LogStream::noise,LogCode::rmt_raw_capture,
        i+503,1,LogSeverity::info,1,1,0,kUnified433Capture);
    assert(seq && detail->save_rf(true,1,keybase|seq,i+503,
        433920000,&glitch,nullptr,1));
  }
  const auto after=detail->stats();
  assert(after.entries[1]==501 && after.evicted[1]==0);
  assert(after.entries[3]==2000 && after.evicted[3]==100);
  auto snapshot=std::make_unique<DetailSnapshot>();
  assert(detail->copy(1,1,keybase|message,snapshot.get()));
  RfDetailMeta meta{};
  std::memcpy(&meta,snapshot->bytes.data(),sizeof(meta));
  assert(meta.count==49 && std::memcmp(snapshot->bytes.data()+sizeof(meta),
      original,sizeof(original))==0);
  const auto stats=log->stats();const auto counters=log->rx_counts();
  assert(stats.used[1]==1 && stats.capture_used[1]==500);
  assert(stats.used[3]==2000 && stats.overwritten[3]==100);
  assert(counters.messages[1]==1 && counters.candidates[1]==500);
  LogRecord row{};
  LogQuery q{};q.radio=1;q.stream=0;
  assert(log->page_after(&row,1,0,log->latest_sequence(),q)==1);
  assert(row.sequence==message && row.count==49 && row.rx_decoder_count==50);
  assert(row.correlation_id==100 && row.flags&kUnified433Capture);
  // Ambiguous user match is isolated from confirmed protocol messages.
  log->push(LogStream::signal,LogCode::user_rx_ambiguous,
      3000,1,LogSeverity::warning,49,0,700,kUnified433Capture);
  assert(log->stats().used[1]==1 && log->stats().capture_overwritten[1]==1);
  assert(log->rx_counts().messages[1]==1 && log->rx_counts().candidates[1]==501);
}
'''

class UnifiedRxRetention(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "g++ required")
    def test_policy_and_original_evidence(self):
        with tempfile.TemporaryDirectory() as temp:
            source=Path(temp)/"test.cpp"
            binary=Path(temp)/"test"
            source.write_text(CPP)
            build=subprocess.run(["g++","-std=c++17","-pthread","-Wall","-Wextra",
                                  "-Werror","-I"+str(MAIN),str(source),"-o",str(binary)],
                                  capture_output=True,text=True,timeout=45)
            self.assertEqual(build.returncode,0,build.stderr)
            ran=subprocess.run([str(binary)],capture_output=True,text=True,timeout=20)
            self.assertEqual(ran.returncode,0,ran.stderr)

    def test_worker_dispatch_and_export_contract(self):
        main=(MAIN/"main.cpp").read_text()
        observer=main.split("static void log_rx_observer(",1)[1].split("static void log_rx_detail(",1)[0]
        self.assertLess(observer.index("if(pending433.active"),observer.index("log->push("))
        self.assertIn("return; // One finalized record",observer)
        self.assertIn("if(pending433.active && pending433.radio==id && pulses)return;",main)
        self.assertIn("std::memcpy(pending433_pulses,pulses,count*sizeof(int32_t));",main)
        finalize=main.split("static void finalize_433_capture(){",1)[1].split("// This observer is deliberately",1)[0]
        self.assertEqual(finalize.count("event_logs->push("),1)
        self.assertEqual(finalize.count("detail_logs->save_rf("),1)
        self.assertIn("detail_logs->save_rf(noise,radio,",finalize)
        self.assertIn("(reason!=2 && reason!=4)",main)
        web=(MAIN/"rflink_web_ota.cpp").read_text()
        self.assertIn("const bool pre=e.code==46 || sync || unified;",web)
        self.assertIn("detail_provider->copy(3,radio,key,snapshot)",web)
        self.assertIn("e.rx_decoder_count",web)
        for root in (MAIN/"webui",ROOT/"rflink_builder/static/rflink_web"):
            js=(root/"app.js").read_text()
            self.assertIn("e.radio<2&&!!(Number(e.flags)&128)",js)
            self.assertIn("const activity=[s.rx43342_frames,s.rx43392_frames,null]",js)
            self.assertIn("e.rx_decoder_count",js)
            html=(root/"index.html").read_text()
            for i in range(3):
                self.assertIn(f'id="radio-activity-{i}"',html)

if __name__=="__main__":
    unittest.main()
