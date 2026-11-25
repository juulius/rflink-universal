"""Observational RF features, calculated from retained immutable snapshots."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
MAIN=ROOT/"firmware/main"

CPP=r'''
#include "rflink_signal_observation.hpp"
#include <cassert>
#include <cstring>
#include <limits>
using namespace rflink;
int main(){
  const int32_t regular[]={320,-640,340,-630,320,-640,340,-630,320,-640,340,-630};
  // The evidence may be unaligned. memcpy must preserve signed durations.
  std::array<uint8_t,sizeof(regular)+1> data{};
  std::memcpy(data.data()+1,regular,sizeof(regular));
  auto p=observe_pulses(data.data()+1,12);
  assert(p.total_us==5790 && p.min_us==320 && p.max_us==640);
  assert(p.high==6 && p.low==6 && p.level_changes==11);
  assert(p.timing_samples==12 && p.top_two_support==12);
  assert(p.high_modes==2 && p.low_modes==1);
  assert(std::strcmp(p.pattern(),"concentrated_timing")==0);
  const int32_t varied[]={100,-200,400,-600,800,-1000,1200,-1400,1600,-1800,2000,-2200};
  p=observe_pulses(reinterpret_cast<const uint8_t*>(varied),12);
  assert(p.timing_samples==12 && p.top_two_support==4);
  assert(std::strcmp(p.pattern(),"varied_timing")==0);
  const int32_t short_input[]={20,-25,0,100,-5000,std::numeric_limits<int32_t>::min()};
  p=observe_pulses(reinterpret_cast<const uint8_t*>(short_input),6);
  assert(p.short_durations==2 && p.zero==1 && p.outside_bins==2);
  assert(p.total_us==2147488793ull && p.max_us==2147483648u);
  assert(p.timing_samples==3 && p.top_two_support==1);
  assert(std::strcmp(p.pattern(),"insufficient_timing")==0);
  assert(observe_pulses(nullptr,1024).timing_samples==0);
  assert(observe_pulses(data.data(),1025).timing_samples==0);

  std::array<uint8_t,63> bits{};
  for(unsigned i=0;i<4;++i)bits[i]=0xaa;
  auto b=observe_clocked_bits(bits.data(),bits.size());
  assert(b.bits==504 && b.tail_bits==472 && b.ones==16);
  assert(b.alternating_prefix==32 && b.transitions==31 && b.longest_run==473);
  assert(b.tail_ones==0 && b.tail_transitions==0 && b.tail_longest_run==472);
  assert(std::strcmp(b.pattern(),"long_constant_run")==0);
  bits.fill(0x55);
  b=observe_clocked_bits(bits.data(),bits.size());
  assert(b.alternating_prefix==504 && b.transitions==503 && b.ones==252);
  assert(b.tail_ones==236 && b.tail_transitions==471 && b.tail_longest_run==1);
  assert(std::strcmp(b.pattern(),"predominantly_alternating")==0);
  bits.fill(0x33);
  for(unsigned i=0;i<4;++i)bits[i]=0xaa;
  b=observe_clocked_bits(bits.data(),bits.size());
  assert(b.alternating_prefix==32 && b.tail_longest_run==2 && b.tail_ones==236);
  assert(std::strcmp(b.pattern(),"mixed_bits")==0);
  assert(std::strcmp(observe_clocked_bits(bits.data(),4).pattern(),"insufficient_tail")==0);
  assert(observe_clocked_bits(bits.data(),64).bits==0);
}
'''

class SignalObservationTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"),"g++ required")
    def test_pulses_and_bits(self):
        with tempfile.TemporaryDirectory() as d:
            src=Path(d)/"test.cpp"; exe=Path(d)/"test"
            src.write_text(CPP)
            built=subprocess.run(["g++","-std=c++17","-Wall","-Wextra","-Werror",
                                  "-I"+str(MAIN),str(src),"-o",str(exe)],
                                  capture_output=True,text=True,timeout=45)
            self.assertEqual(built.returncode,0,built.stderr)
            ran=subprocess.run([str(exe)],capture_output=True,text=True,timeout=15)
            self.assertEqual(ran.returncode,0,ran.stderr)

    def test_observations_only_in_existing_read_paths(self):
        web=(MAIN/"rflink_web_ota.cpp").read_text()
        self.assertIn("signal_observation_json(*detail,meta,requested_sync)",web)
        self.assertIn("signal_observation_json(*snapshot,meta,sync)",web)
        self.assertIn("clocked && detail.format==rflink::DetailFormat::packet && meta.count==63",web)
        self.assertNotIn("rflink_signal_observation.hpp",(MAIN/"main.cpp").read_text())
        for root in (MAIN/"webui",ROOT/"rflink_builder/static/rflink_web"):
            js=(root/"app.js").read_text()
            self.assertEqual(js.count("signalObservationText(frame.observation)"),2)
            self.assertIn("No carrier or RSSI is attributed to this window yet.",js)
            self.assertIn("Timing concentration is not proof of a valid message.",js)
            self.assertIn("capture:raw",js)

if __name__=="__main__":
    unittest.main()
