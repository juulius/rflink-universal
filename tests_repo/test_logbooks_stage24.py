"""Host checks for the bounded native v0.7 signal/noise/system/web logbook.

A native toolchain compilation and on-board SRAM/PSRAM testing are separate gates.
"""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
FW = ROOT / "firmware"


class LogbooksStage24Tests(unittest.TestCase):
    def test_ring_retention_noise_filter_and_atomic_resize(self):
        source = r"""
#include "rflink_log_store.hpp"
#include <cassert>
#include <cstdlib>
using namespace rflink;
int allocated=0;
bool fail_alloc=false;
void* alloc(size_t n,size_t s){if(fail_alloc && ++allocated>=2)return nullptr;return std::calloc(n,s);}
void release(void* p){std::free(p);}
int main(){
  LogStore logs(alloc,release);
  const LogLimits defaults{};
  assert(defaults.signal[0]==250&&defaults.signal[1]==250&&defaults.signal[2]==250);
  assert(defaults.noise==2000);
  assert(LogStore::bytes_for(defaults)<=kLogBudgetBytes);
  assert(logs.configure(defaults));
  assert(logs.stats().ready);
  assert(logs.stats().used[0]==0);
  assert(logs.stats().allocated_bytes==LogStore::bytes_for(defaults));
  logs.push(LogStream::signal,LogCode::itho,10,2);
  logs.push(LogStream::noise,LogCode::raw_pulses,11,0);
  logs.push(LogStream::noise,LogCode::raw_pulses,12,1);
  logs.push(LogStream::system,LogCode::rx_recovered,13,1);
  logs.push(LogStream::web,LogCode::settings_saved,14,kAnyRadio);
  LogRecord recent[32]{};
  size_t n=logs.recent(recent,32);
  assert(n==3);                       // Default "All" hides noise.
  assert(recent[0].code==static_cast<uint16_t>(LogCode::itho));
  assert(recent[1].code==static_cast<uint16_t>(LogCode::rx_recovered));
  assert(recent[2].code==static_cast<uint16_t>(LogCode::settings_saved));
  n=logs.recent(recent,32,LogQuery{-1,-1,true});
  assert(n==5);
  assert(recent[0].sequence==1 && recent[4].sequence==5);
  n=logs.recent(recent,32,LogQuery{1,1,false});
  assert(n==1 && recent[0].radio==1 && recent[0].stream==1);
  n=logs.recent(recent,32,LogQuery{-1,0,true});
  assert(n==1 && recent[0].stream==1); // Single shared noise ring, filtered by radio.
  n=logs.recent(recent,2,LogQuery{-1,-1,true});
  assert(n==2 && recent[0].sequence==4 && recent[1].sequence==5);
  LogLimits smaller{};
  smaller.signal[0]=2;smaller.signal[1]=2;smaller.signal[2]=2;smaller.noise=2;
  assert(logs.configure(smaller));
  for(int i=0;i<7;i++)logs.push(LogStream::signal,LogCode::ramses,100+i,0);
  assert(logs.stats().used[0]==2);
  assert(logs.stats().overwritten[0]==5);
  n=logs.recent(recent,32,LogQuery{0,0,false});
  assert(n==2 && recent[0].uptime_ms==105 && recent[1].uptime_ms==106);
  assert(logs.configure(defaults)); // Upsizing retains the newest events.
  n=logs.recent(recent,32,LogQuery{0,0,false});
  assert(n==2 && recent[0].uptime_ms==105);
  LogLimits huge{};huge.signal[0]=4001;huge.signal[1]=4000;huge.signal[2]=4000;huge.noise=10000;
  assert(!LogStore::valid_limits(huge)); // Per-radio maximum enforced.
  huge.signal[0]=4000;
  assert(LogStore::valid_limits(huge)); // Still within the 1.5 MiB hard ceiling.
  huge.signal[0]=4001;
  assert(!logs.configure(huge));
  assert(logs.stats().limits.signal[0]==250);
  allocated=0;fail_alloc=true;
  assert(!logs.configure(smaller));
  assert(logs.stats().limits.signal[0]==250); // Allocator failure is atomic.
  fail_alloc=false;
  assert(logs.configure(LogLimits{ {0,2,2}, 0 }));
  logs.push(LogStream::signal,LogCode::ramses,200,0);
  assert(logs.stats().used[0]==0);
  assert(logs.stats().overwritten[0]>=6);
  assert(logs.recent(nullptr,8)==0);
  assert(logs.recent(recent,32,LogQuery{12,-1,false})==0);
}
"""
        with tempfile.TemporaryDirectory(prefix="rflink-log-test-") as temp:
            path = Path(temp)
            (path / "check.cpp").write_text(source)
            cmd = ["g++", "-std=c++17", "-O2", "-pthread", "-Wall",
                   "-Wextra", "-Werror", "-I" + str(FW / "main"),
                   str(path / "check.cpp"), "-o", str(path / "check")]
            compiled = subprocess.run(cmd, capture_output=True, text=True, timeout=50)
            self.assertEqual(compiled.returncode, 0, compiled.stderr)
            ran = subprocess.run([str(path / "check")], capture_output=True,
                                 text=True, timeout=20)
            self.assertEqual(ran.returncode, 0, ran.stderr)

    def test_native_log_route_and_live_settings_are_bounded(self):
        main = (FW / "main/main.cpp").read_text()
        web = (FW / "main/rflink_web_ota.cpp").read_text()
        log = (FW / "main/rflink_log_store.hpp").read_text()
        self.assertIn("log_psram_alloc", main)
        self.assertIn("set_event_observer", main)
        self.assertIn("rflink_set_log_store_provider", main)
        self.assertIn("LogStore::valid_limits", web)
        self.assertIn('"/api/logs"', web)
        self.assertIn('"/api/logs/config"', web)
        self.assertIn("if(!key_ok(req))", web)
        self.assertIn("nvs_set_blob", web)
        self.assertIn("kLogBudgetBytes=1536*1024", log)
        self.assertIn("kSystemLogRetention=128", log)
        self.assertIn("kWebLogRetention=128", log)
        self.assertNotIn("raw_payload", log)


if __name__ == "__main__":
    unittest.main()
