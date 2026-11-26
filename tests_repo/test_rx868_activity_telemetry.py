"""Passive 868 carrier-period telemetry: host tests and transport contracts."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
HAL=ROOT/"firmware/components/rflink_radio_hal"
MAIN=ROOT/"firmware/main"

CPP=r'''
#include "rflink_rx_activity.hpp"
#include <cassert>
#include <vector>
using namespace rflink;
static void retain(void* ctx,const RxActivityPeriod& p){
  static_cast<std::vector<RxActivityPeriod>*>(ctx)->push_back(p);
}
int main(){
  RxActivityTracker t;
  std::vector<RxActivityPeriod> periods;
  t.set_period_observer(retain,&periods);
  static_assert(RxActivityTracker::kSampleIntervalUs==5000);
  assert(!t.stats().valid && t.stats().bursts==0);
  assert(RxActivityTracker::approximate_rssi_x2(0)==-148);
  assert(RxActivityTracker::approximate_rssi_x2(0x80)==-276);
  assert(RxActivityTracker::approximate_rssi_x2(0xff)==-149);
  t.observe(10000,false,0x90,30);
  t.observe(20000,true,0x14,40);
  t.observe(30000,true,0x18,60);
  t.observe(40000,false,0x91,50);
  t.observe(50000,true,0x15,40); // one low sample does not split period
  assert(t.stats().bursts==1 && t.stats().active);
  t.observe(60000,false,0x90,40);
  t.observe(70000,false,0x90,40);
  assert(!t.stats().active);
  t.observe(80000,true,0x15,50);
  assert(t.stats().bursts==2 && t.stats().partial_starts==0);
  t.missed(100);
  assert(!t.stats().valid && t.stats().missed_samples==1);
  t.observe(120000,true,0x10,45);
  auto s=t.stats();
  assert(s.bursts==3 && s.partial_starts==1 && s.gap_resets==1);
  assert(s.max_gap_us==40000 && s.max_read_us==100);
  assert(s.samples==9 && s.carrier_samples==5);
  assert(s.min_rssi_dbm_x2==-260 && s.max_rssi_dbm_x2==-124);
  t.reset_window(); // TX/stop/restart: never join periods across epochs
  assert(!t.stats().valid && !t.stats().active);
  t.observe(900000,true,0x12,30);
  assert(t.stats().bursts==4 && t.stats().partial_starts==2);
  assert(t.stats().gap_resets==1 && t.stats().max_gap_us==40000);
  t.observe(910000,false,0x90,30);
  t.observe(920000,false,0x90,30);
  for(unsigned i=0;i<1000;++i)t.observe(930000+i*10000,false,0x90,30);
  assert(t.stats().bursts==4 && !t.stats().active);
  assert(periods.size()==4 && t.stats().completed_periods==4);
  const auto& a=periods[0];
  assert(a.id==1 && a.first_high_us==20000 && a.last_high_us==50000);
  assert(a.high_samples==3 && a.max_rssi_x2==-124 && a.min_rssi_x2==-128);
  assert(!a.partial_start && a.end==RxActivityEnd::low);
  assert(periods[1].id==2 && periods[1].high_samples==1 &&
         periods[1].missed_samples==1 && periods[1].end==RxActivityEnd::gap);
  assert(periods[2].id==3 && periods[2].partial_start &&
         periods[2].end==RxActivityEnd::stopped);
  assert(periods[3].id==4 && periods[3].partial_start &&
         periods[3].end==RxActivityEnd::low);
  assert(t.stats().read_time_us>0 && t.stats().valid_gap_count>1000);
  t.reset_window(); // An idle reset never creates another record.
  assert(periods.size()==4);
}
'''

class RxActivityTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"),"g++ required")
    def test_sampled_periods_and_gaps(self):
        with tempfile.TemporaryDirectory() as temp:
            src=Path(temp)/"test.cpp";exe=Path(temp)/"test"
            src.write_text(CPP)
            built=subprocess.run(["g++","-std=c++17","-Wall","-Wextra","-Werror",
                                  "-I"+str(HAL/"include"),str(src),"-o",str(exe)],
                                 capture_output=True,text=True,timeout=45)
            self.assertEqual(built.returncode,0,built.stderr)
            ran=subprocess.run([str(exe)],capture_output=True,text=True,timeout=15)
            self.assertEqual(ran.returncode,0,ran.stderr)
    def test_only_worker_samples_status_without_modem_changes(self):
        spi=(HAL/"rflink_cc1101_spi.cpp").read_text()
        sample=spi.split("Cc1101SpiBus::try_read_rx_activity(",1)[1].split(
            "Cc1101SpiBus::read_runtime_status(",1)[0]
        self.assertIn("read_status_register(id,0x34,&raw,true)",sample)
        self.assertIn("read_status_register(id,0x38,&status,true)",sample)
        self.assertIn("(status&0x40)!=0",sample)
        self.assertNotIn("strobe(",sample)
        self.assertNotIn("write_register(",sample)
        self.assertIn("xSemaphoreTake(lock_,0)",spi)
        self.assertIn("opportunistic?50u:kReadyTimeoutUs",spi)
        rx=(HAL/"rflink_sync_868_bench_rx.cpp").read_text()
        worker=rx.split("Cc1101Sync868BenchRx::poll_once()",1)[1].split(
            "Cc1101Sync868BenchRx::sample_activity()",1)[0]
        self.assertLess(worker.index("collect_completed()"),worker.index("sample_activity()"))
        self.assertIn("if(!listening_)return;",worker)
        self.assertIn("activity_.missed(elapsed)",rx)
        self.assertEqual(rx.count("activity_.reset_window()"),2)
        web=(MAIN/"rflink_web_ota.cpp").read_text()
        self.assertIn("const auto& activity=sync_rx.activity",web)
        self.assertIn("activity.max_gap_us",web)
        self.assertIn("activity.max_read_us",web)
        self.assertNotIn("try_read_rx_activity(",web)
        self.assertIn("activity.valid_gap_sum_us",web)
        self.assertIn("activity.read_time_us",web)
        self.assertIn("rflink-carrier-period-v1",web)
        main=(MAIN/"main.cpp").read_text()
        self.assertIn("bench_sync_rx.set_activity_observer(&log_sync868_activity,nullptr)",main)
        self.assertIn("LogStream::noise,rflink::LogCode::rx868_carrier_period",main)
        self.assertIn("vTaskDelay(kPollDelayTicks)",main)
        for root in (MAIN/"webui",ROOT/"rflink_builder/static/rflink_web"):
            js=(root/"app.js").read_text()
            self.assertIn("a868?.samples>0?a868.bursts:null",js)
            self.assertIn("s.rx868_activity.missed_samples",js)

if __name__=="__main__":
    unittest.main()
