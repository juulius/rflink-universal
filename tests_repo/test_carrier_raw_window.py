"""Bounded carrier evidence: exact bytes, timing limits and gap handling."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
HAL=ROOT/"firmware/components/rflink_radio_hal/include"
CPP=r'''
#include "rflink_carrier_raw_window.hpp"
#include <cassert>
using namespace rflink;
struct Sink {
  CarrierRawWindow last{};
  unsigned n=0;
  bool accept=true;
};
bool save(void* context,const CarrierRawWindow& w){
  auto& s=*static_cast<Sink*>(context);
  s.last=w;++s.n;return s.accept;
}
ClockedBitBlock block(uint64_t start,uint32_t epoch=1){
  ClockedBitBlock b{};
  b.epoch=epoch;b.bit_count=2048;
  b.first_sample_us=start;b.last_sample_us=start+2047*25;
  for(unsigned i=0;i<b.bytes.size();++i)b.bytes[i]=static_cast<uint8_t>(i);
  return b;
}
int main(){
  CarrierRawRecorder rec;Sink sink;
  rec.set_observer(save,&sink);
  rec.consume(block(100000),1);
  rec.sample(1,150000,true,true,true,1);
  rec.sample(1,160000,true,true,true,1);
  rec.sample(1,170000,true,false,true,1);
  rec.sample(1,180000,true,false,false,1);
  assert(sink.n==0);
  rec.consume(block(151200),2);
  assert(sink.n==1 && rec.stats().queued==1);
  const auto w=sink.last;
  assert(w.activity_id==1 && w.epoch==1 && w.trigger_us==150000);
  assert(w.first_us==120000 && w.last_us==179975);
  assert(w.size==300 && !w.short_history && !w.limited);
  for(unsigned i=0;i<w.size;++i)assert(w.bytes[i]==static_cast<uint8_t>(100+i));
  rec.sample(1,190000,true,false,false,1);
  assert(sink.n==1);
  rec.sample(1,210000,true,true,true,2);
  auto gap=block(202400);gap.discontinuity_before=true;
  rec.consume(gap,3);
  assert(rec.stats().aborted==1);
  rec.sample(1,220000,true,true,true,2);
  assert(rec.stats().started==2);
  rec.reset();
  rec.consume(block(300000,2),4);
  rec.sample(2,310000,true,true,true,3);
  rec.sample(2,360000,true,true,true,3);
  sink.accept=false;
  rec.consume(block(351200,2),5);
  assert(sink.n==2 && sink.last.limited && sink.last.short_history);
  assert(sink.last.size<=384 && rec.stats().queue_dropped==1);
  rec.sample(2,370000,true,true,true,3);
  assert(rec.stats().started==3);
  rec.sample(2,410000,true,true,true,4);
  rec.sample(2,430000,true,false,false,4);
  rec.reset();
  assert(rec.stats().aborted==2);
}
'''
class CarrierRawTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"),"g++ required")
    def test_exact_bytes_and_bounds(self):
        with tempfile.TemporaryDirectory() as d:
            src=Path(d)/"test.cpp";exe=Path(d)/"test"
            src.write_text(CPP)
            run=subprocess.run(["g++","-std=c++17","-Wall","-Wextra","-Werror",
                "-I"+str(HAL),str(src),"-o",str(exe)],capture_output=True,text=True,timeout=45)
            self.assertEqual(run.returncode,0,run.stderr)
            run=subprocess.run([str(exe)],capture_output=True,text=True,timeout=15)
            self.assertEqual(run.returncode,0,run.stderr)
    def test_retention_and_nonblocking_handoff(self):
        main=(ROOT/"firmware/main/main.cpp").read_text()
        self.assertIn("xQueueSend(carrier_raw_queue,&window,0)",main)
        self.assertIn("xQueueCreate(2,sizeof(rflink::CarrierRawWindow))",main)
        web=(ROOT/"firmware/main/rflink_web_ota.cpp").read_text()
        self.assertIn("carrier_bits_hex",web)
        self.assertIn("rx868_carrier_raw",web)
        for root in ("firmware/main/webui","rflink_builder/static/rflink_web"):
            js=(ROOT/root/"app.js").read_text()
            self.assertIn("rawSync=e.code===55||e.code===57",js)
            self.assertIn("carrier_bits_hex",js)

if __name__=="__main__":
    unittest.main()
