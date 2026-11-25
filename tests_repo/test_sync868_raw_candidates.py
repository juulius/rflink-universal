"""Host-only synthetic RAW candidate and dashboard counting tests."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
MAIN = ROOT / "firmware/main"
HAL = ROOT / "firmware/components/rflink_radio_hal/include"

CPP = r'''
#include "rflink_sync868_raw_candidate.hpp"
#include "rflink_log_store.hpp"
#include <cassert>
#include <vector>
using namespace rflink;
using Bits=std::vector<bool>;
using Hits=std::vector<Sync868RawCandidate>;
static void retain(void* ctx,const Sync868RawCandidate& c){
  static_cast<Hits*>(ctx)->push_back(c);
}
static ClockedBitBlock block(const Bits& bits,uint32_t epoch=1){
  ClockedBitBlock b{};
  assert(bits.size()<=b.kBitCapacity);
  b.bit_count=static_cast<uint16_t>(bits.size());b.epoch=epoch;
  for(size_t i=0;i<bits.size();++i)
    if(bits[i])b.bytes[i/8]|=uint8_t(0x80u>>(i%8));
  return b;
}
static Bits signal(){
  Bits b;
  for(unsigned i=0;i<32;++i)b.push_back((i&1u)!=0);
  for(unsigned i=32;i<504;++i)b.push_back(i%13==0);
  return b;
}
int main(){
  const auto sample=signal();
  const Bits first(sample.begin(),sample.begin()+19);
  const Bits rest(sample.begin()+19,sample.end());
  {
    Sync868RawCandidateDetector d;Hits h;
    d.consume(block(Bits(2048,false)),1,retain,&h);
    assert(h.empty());
    d.consume(block(first),2,retain,&h);
    d.consume(block(rest),3,retain,&h);
    assert(h.size()==1 && h[0].epoch==1);
    for(size_t i=0;i<sample.size();++i)
      assert(bool(h[0].bytes[i/8]&(0x80u>>(i%8)))==sample[i]);
    d.consume(block(sample),4,retain,&h);
    assert(h.size()==1); // holdoff
    for(unsigned i=5;i<10;++i)d.consume(block(Bits(2048,false)),i,retain,&h);
    d.consume(block(sample),10,retain,&h);
    assert(h.size()==2);
  }
  for(unsigned kind=0;kind<4;++kind){
    Sync868RawCandidateDetector d;Hits h;
    auto a=block(first);auto b=block(rest);
    if(kind==1)b.epoch=2;
    if(kind==2)b.discontinuity_before=true;
    if(kind==3)a.window_end=true;
    d.consume(a,1,retain,&h);
    d.consume(b,kind==0?3:2,retain,&h);
    assert(h.empty()); // never stitch lost/other epoch/window fragments
  }
  {
    Sync868RawCandidateDetector d;Hits h;
    Bits alternating(2048);
    for(unsigned i=0;i<alternating.size();++i)alternating[i]=bool(i&1);
    for(unsigned i=1;i<8;++i)d.consume(block(alternating),i,retain,&h);
    assert(h.size()==1); // one continuous preamble does not flood the log
  }
  {
    LogStore l;assert(l.configure({}));
    l.push(LogStream::signal,LogCode::rx868_sync_ramses_valid,1,2);
    l.push(LogStream::signal,LogCode::rx868_sync_itho_candidate,2,2);
    l.push(LogStream::signal,LogCode::acurite_609txc,3,1);
    l.push(LogStream::signal,LogCode::rmt_raw_capture,4,1);
    l.push(LogStream::noise,LogCode::rmt_raw_capture,5,0);
    l.push(LogStream::signal,LogCode::tx_driver_complete,6,2);
    for(unsigned i=0;i<1600;++i)
      l.push(LogStream::signal,LogCode::rx868_sync_raw_candidate,7+i,2);
    auto s=l.stats();auto c=l.rx_counts();
    assert(s.used[2]==3 && s.unknown868_used==500 && s.unknown868_overwritten==1100);
    assert(c.messages[0]==0 && c.messages[1]==1 && c.messages[2]==1);
    assert(c.candidates[0]==0 && c.candidates[1]==1 && c.candidates[2]==1600);
    LogLimits limits{};limits.signal[2]=1;assert(l.configure(limits));
    assert(l.rx_counts().messages[2]==1 && l.rx_counts().candidates[2]==1600);
  }
}
'''

class RawCandidateTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++") or shutil.which("clang++"), "C++17 compiler required")
    def test_exact_windows_boundaries_and_isolated_retention(self):
        compiler = shutil.which("g++") or shutil.which("clang++")
        with tempfile.TemporaryDirectory() as temp:
            source = Path(temp) / "test.cpp"
            binary = Path(temp) / "test"
            source.write_text(CPP)
            run = subprocess.run([compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror",
                                  "-pthread", "-I"+str(MAIN), "-I"+str(HAL), str(source),
                                  "-o", str(binary)], capture_output=True, text=True, timeout=60)
            self.assertEqual(run.returncode, 0, run.stderr)
            run = subprocess.run([str(binary)], capture_output=True, text=True, timeout=10)
            self.assertEqual(run.returncode, 0, run.stderr)

    def test_ordinary_raw_payload_and_common_dashboard(self):
        web=(MAIN/"rflink_web_ota.cpp").read_text()
        main=(MAIN/"main.cpp").read_text()
        capture=web.split("static esp_err_t raw_capture_get(",1)[1].split("static esp_err_t",1)[0]
        self.assertNotIn("key_ok(req)",capture)
        self.assertIn("if(include_sync868 && !key_ok(req))",web)
        self.assertNotIn("csv_export_sync_candidates",web)
        self.assertIn("export_copy_detail(e,detail,&row[20],&row[21],&row[22]);",web)
        self.assertIn("const bool rx=(e.code>=1 && e.code<=8) || e.code==46 || sync || unified;",web)
        self.assertIn("clocked_bits_hex",web)
        self.assertIn("kSyncKey|seq",main)
        self.assertIn("sync868_raw_detector.consume",main)
        for directory in (MAIN/"webui", ROOT/"rflink_builder/static/rflink_web"):
            js=(directory/"app.js").read_text()
            html=(directory/"index.html").read_text()
            self.assertIn("const frames=s.rx_messages||[]",js)
            self.assertIn("const candidates=s.rx_raw_candidates||[]",js)
            self.assertNotIn("RX bit blocks (not packets)",js)
            for i in range(3):
                self.assertIn(f'id="radio-candidates-{i}"',html)
            self.assertIn("const rawSync=e.code===55",js)
            self.assertIn("raw.dataset.rawSync",js)
            self.assertIn("exportButton.dataset.exportRfSync",js)
            self.assertIn("sync?'&sync=1':pre?'&pre=1':''",js)
            self.assertIn("frame.format==='clocked_bits_hex'",js)
            self.assertNotIn("Read protected RAW bits",js)

if __name__=="__main__":
    unittest.main()
