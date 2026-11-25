"""Bounded, epoch-aware carrier context on automatic synchronous RAW windows."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
MAIN=ROOT/"firmware/main"
HAL=ROOT/"firmware/components/rflink_radio_hal"

CPP=r'''
#include "rflink_sync868_raw_candidate.hpp"
#include "rflink_rx_carrier_history.hpp"
#include "rflink_log_details.hpp"
#include <cassert>
#include <cstring>
#include <memory>
#include <vector>
using namespace rflink;

static ClockedBitBlock block(const std::vector<bool>& bits,uint64_t start,uint32_t epoch=1){
  ClockedBitBlock b{};
  b.epoch=epoch;b.bit_count=static_cast<uint16_t>(bits.size());
  b.first_sample_us=start;b.last_sample_us=start+25u*(bits.size()-1);
  for(size_t i=0;i<bits.size();++i)
    if(bits[i])b.bytes[i/8]|=uint8_t(0x80u>>(i%8));
  return b;
}
static void retain(void* context,const Sync868RawCandidate& raw){
  static_cast<std::vector<Sync868RawCandidate>*>(context)->push_back(raw);
}
int main(){
  std::vector<bool> first(40,false),rest(488,false);
  for(unsigned i=0;i<16;++i){first[24+i]=(i%2)==0;rest[i]=(i%2)==0;}
  Sync868RawCandidateDetector d;
  std::vector<Sync868RawCandidate> captured;
  d.consume(block(first,1000),1,retain,&captured);
  d.consume(block(rest,2000),2,retain,&captured);
  assert(captured.size()==1);
  const auto& raw=captured.front();
  assert(raw.timing_valid && raw.epoch==1);
  assert(raw.estimated_start_us==1600 && raw.estimated_end_us==14175);
  assert(raw.bytes[0]==0xaa && raw.bytes[3]==0xaa && raw.bytes[4]==0);
  RxCarrierHistory history;
  history.append(1,1500,1550,true,true,-140,6);
  history.append(1,1599,1601,true,true,-145,6); // crosses start: exclude
  history.append(1,1700,1750,true,true,-180,7);
  history.append(1,5000,5050,false,false,0,0);
  history.append(2,6500,6550,true,true,-100,8); // other epoch: exclude
  history.append(1,11000,11060,true,false,-200,7);
  history.append(1,14150,14200,true,true,-130,9); // crosses end: exclude
  const auto request=carrier_context_request(raw.epoch,raw.estimated_start_us,
      raw.estimated_end_us,raw.timing_valid);
  auto c=history.correlate(request);
  assert(c.state==CarrierContextState::sampled && !c.history_starts_late);
  assert(c.samples==2 && c.carrier_samples==1 && c.missed_samples==1);
  assert(c.first_sample_us==1700 && c.last_sample_us==11060);
  assert(c.min_rssi_x2==-200 && c.max_rssi_x2==-180);
  assert(c.first_burst==7 && c.last_burst==7);
  const auto absent=history.correlate(carrier_context_request(3,1600,14175,true));
  assert(absent.state==CarrierContextState::no_matching_samples && absent.history_starts_late);
  assert(absent.samples==0 && absent.carrier_samples==0);
  const auto invalid=history.correlate(carrier_context_request(1,200,100,true));
  assert(invalid.state==CarrierContextState::invalid_timing);
  const auto missed=history.correlate(carrier_context_request(1,4900,5100,true));
  assert(missed.state==CarrierContextState::no_matching_samples && missed.missed_samples==1);
  for(unsigned i=0;i<64;++i)history.append(1,30000+i*10000,30050+i*10000,true,false,-220,10);
  const auto expired=history.correlate(request);
  assert(expired.state==CarrierContextState::no_matching_samples && expired.history_starts_late);
  assert(expired.samples==0);
  Sync868RawCandidateDetector invalid_clock;
  captured.clear();
  auto a=block(first,1000);a.last_sample_us=a.first_sample_us;
  invalid_clock.consume(a,1,retain,&captured);
  invalid_clock.consume(block(rest,2000),2,retain,&captured);
  assert(captured.size()==1 && !captured[0].timing_valid);

  auto details=std::make_unique<DetailStore>();
  assert(details->configure());
  constexpr uint64_t key=(uint64_t(3)<<62)|7;
  // Use a separate array: captured vector was cleared above.
  uint8_t bytes[63]{};bytes[0]=0xaa;
  assert(details->save_rf(true,2,key,50,868300000,nullptr,bytes,63,false,0,0,0,&c,sizeof(c)));
  auto snap=std::make_unique<DetailSnapshot>();
  assert(details->copy(6,2,key,snap.get()));
  assert(snap->size==sizeof(RfDetailMeta)+63+sizeof(c));
  RfDetailMeta meta{};
  std::memcpy(&meta,snap->bytes.data(),sizeof(meta));
  assert(meta.count==63 && !meta.has_quality);
  assert(std::memcmp(snap->bytes.data()+sizeof(meta),bytes,63)==0);
  CaptureCarrierContext copied{};
  std::memcpy(&copied,snap->bytes.data()+sizeof(meta)+63,sizeof(copied));
  assert(copied.samples==2 && copied.first_burst==7);
  assert(!details->save_rf(true,2,8,50,868300000,nullptr,bytes,63,false,0,0,0,&c,sizeof(c)));
  assert(!details->save_rf(false,2,key,50,868300000,nullptr,bytes,63,false,0,0,0,&c,sizeof(c)));
  assert(details->save_rf(true,2,key+1,50,868300000,nullptr,bytes,63));
  assert(details->copy(6,2,key+1,snap.get()));
  RfDetailMeta ordinary{};
  std::memcpy(&ordinary,snap->bytes.data(),sizeof(ordinary));
  assert(snap->size==sizeof(RfDetailMeta)+63 && ordinary.hash==meta.hash);
}
'''

class CaptureCarrierContextTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"),"g++ required")
    def test_history_capture_timing_and_retention(self):
        with tempfile.TemporaryDirectory() as d:
            src=Path(d)/"test.cpp"; exe=Path(d)/"test"
            src.write_text(CPP)
            build=subprocess.run(["g++","-std=c++17","-pthread","-Wall","-Wextra","-Werror",
                                  "-I"+str(MAIN),"-I"+str(HAL/"include"),str(src),"-o",str(exe)],
                                 capture_output=True,text=True,timeout=45)
            self.assertEqual(build.returncode,0,build.stderr)
            run=subprocess.run([str(exe)],capture_output=True,text=True,timeout=15)
            self.assertEqual(run.returncode,0,run.stderr)

    def test_existing_transport_and_exports(self):
        rx=(HAL/"rflink_sync_868_bench_rx.cpp").read_text()
        self.assertIn("carrier_history_.append(epoch_,begin,end,result==ESP_OK",rx)
        query=rx.split("Cc1101Sync868BenchRx::try_correlate_activity(",1)[1].split("\n}",1)[0]
        self.assertIn("xSemaphoreTake(guard_,0)",query)
        self.assertNotIn("bus_.",query)
        main=(MAIN/"main.cpp").read_text()
        self.assertIn("rx868sync->try_correlate_activity(carrier)",main)
        self.assertIn("&carrier,sizeof(carrier)",main)
        web=(MAIN/"rflink_web_ota.cpp").read_text()
        self.assertIn("carrier_context_json(*detail,meta)",web)
        self.assertIn("carrier_context_json(*snapshot,meta)",web)
        self.assertIn("interpolated_block_endpoints",web)
        for root in (MAIN/"webui",ROOT/"rflink_builder/static/rflink_web"):
            js=(root/"app.js").read_text()
            self.assertIn("carrierContextText(frame.carrier_context)",js)
            self.assertIn("Not evidence of no RF.",js)
            self.assertIn("capture:raw",js)

if __name__=="__main__":
    unittest.main()
