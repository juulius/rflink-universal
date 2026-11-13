"""CC1101 RX Engine v2: dedicated 433 candidate metadata, never evict decoded signal logs."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]


def read(path):
    return (ROOT/path).read_text(encoding="utf-8")


class CandidateIsolation(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "C++ host compiler required")
    def test_actual_log_store_ring_safety_and_csv_cursor_paging(self):
        source=r"""
#include "rflink_log_store.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
using namespace rflink;
int main(){
  LogStore log;
  LogLimits defaults{};
  assert(log.configure(defaults));
  assert(log.stats().allocated_bytes==LogStore::bytes_for(defaults));
  // Do not alter any of the six original retention limits/NVS fields.
  assert(defaults.signal[0]==250 && defaults.signal[1]==250
      && defaults.signal[2]==250 && defaults.noise==2000);
  assert(kLogRings==6 && kRaw433CaptureRings==2);
  assert(kRaw433CaptureRetention==250);
  assert(log.stats().capture_used[0]==0);
  const auto device=log.push(LogStream::signal,LogCode::user_rx_match,
                             50,1,LogSeverity::info,32);
  assert(device>0);
  const auto other=log.push(LogStream::signal,LogCode::itho,55,2);
  assert(other>device);
  const auto web=log.push(LogStream::web,LogCode::web_page_open,60,kAnyRadio);
  assert(web>other);
  const auto first_raw=log.push(LogStream::signal,LogCode::rmt_raw_capture,61,1,
                                 LogSeverity::info,68);
  assert(first_raw>web);
  for(unsigned i=0;i<400;++i){
    log.push(LogStream::signal,LogCode::rmt_raw_capture,100+i,1,
             LogSeverity::info,12);
    log.push(LogStream::noise,LogCode::unknown_packet,200+i,2);
  }
  auto state=log.stats();
  assert(state.used[1]==1 && state.overwritten[1]==0);
  assert(state.used[2]==1 && state.overwritten[2]==0);
  assert(state.used[5]==1 && state.overwritten[5]==0);
  assert(state.capture_used[0]==0 && state.capture_used[1]==250);
  assert(state.capture_overwritten[1]==151);
  assert(state.used[3]==0 && state.overwritten[3]==0);
  assert(state.unknown868_used==400 && state.unknown868_overwritten==0);
  // Ensure the decoded signal is still visible even if we have much newer
  // RAW evidence, and capture is queryable as stream=signal for compatibility.
  std::array<LogRecord,512> rows{};
  LogQuery rx{};rx.radio=1;rx.stream=0;
  auto count=log.recent(rows.data(),rows.size(),rx);
  assert(count==251);
  assert(rows[0].sequence==device);
  assert(rows[0].code==static_cast<uint16_t>(LogCode::user_rx_match));
  for(size_t i=1;i<count;++i){
    assert(rows[i].code==static_cast<uint16_t>(LogCode::rmt_raw_capture));
    assert(rows[i].sequence>rows[i-1].sequence);
  }
  // CSV export uses page_after() and a frozen sequence cutoff.
  uint64_t cursor=0;
  const uint64_t cutoff=log.latest_sequence();
  size_t sum=0; bool device_found=false;
  LogQuery all{};all.include_noise=true;
  while(true){
    LogRecord page[17]{};
    const auto n=log.page_after(page,17,cursor,cutoff,all);
    if(n==0)break;
    for(size_t i=0;i<n;++i){
      assert(page[i].sequence>cursor);
      if(page[i].sequence==device)device_found=true;
      ++sum;
    }
    cursor=page[n-1].sequence;
  }
  assert(device_found);
  assert(sum==250+400+3); // RAW + noise + two signals + one web.
  // Cursor pagination for Radio tab includes capture metadata once.
  rx.radio_activity=true;rx.stream=-1;
  count=log.page_before(rows.data(),rows.size(),0,cutoff,rx);
  assert(count==251 && rows[0].sequence==device);
  assert(rows[count-1].sequence>device);
  // One capture ring must never spill into another radio's capture buffer.
  const auto first_42=log.push(LogStream::signal,LogCode::rmt_raw_capture,700,0);
  (void)first_42;
  state=log.stats();
  assert(state.capture_used[0]==1 && state.capture_used[1]==250);
  assert(state.capture_overwritten[0]==0);
  // Reconfiguration keeps new separate candidate history and old signal data.
  LogLimits updated=defaults;updated.noise=2;updated.signal[1]=1;
  assert(log.configure(updated));
  state=log.stats();
  assert(state.capture_used[0]==1 && state.capture_used[1]==250);
  assert(state.used[1]==1 && state.overwritten[1]==0);
  assert(state.used[3]==0 && state.overwritten[3]==0);
  assert(state.unknown868_used==400 && state.unknown868_overwritten==0);
  assert(state.allocated_bytes==LogStore::bytes_for(updated));
  return 0;
}"""
        with tempfile.TemporaryDirectory(prefix="rflink-raw-meta-") as d:
            source_file=Path(d)/"test.cpp"
            program=Path(d)/"test"
            source_file.write_text(source,encoding="utf-8")
            compiled=subprocess.run(
                ["g++","-std=c++17","-pthread","-O2","-Wall","-Wextra","-Werror",
                 "-I"+str(ROOT/"firmware/main"),str(source_file),"-o",str(program)],
                capture_output=True,text=True,timeout=45,
            )
            self.assertEqual(compiled.returncode,0,compiled.stderr)
            process=subprocess.run([str(program)],capture_output=True,text=True,timeout=15)
            self.assertEqual(process.returncode,0,process.stderr)

    def test_captured_records_reuse_existing_stream_and_details(self):
        cpp=read("firmware/main/rflink_log_store.hpp")
        main=read("firmware/main/main.cpp")
        web=read("firmware/main/rflink_web_ota.cpp")
        self.assertIn("kRaw433CaptureRings=2",cpp)
        self.assertIn("kTotalLogRings=kLogRings+kRaw433CaptureRings",cpp)
        self.assertIn("code==LogCode::rmt_raw_capture && radio<2",cpp)
        self.assertIn("std::array<Ring,kTotalLogRings> rings_",cpp)
        self.assertIn("std::array<size_t,kTotalLogRings> left",cpp)
        self.assertIn("stats.capture_used",web)
        self.assertIn('"raw433_capture"',web.replace("\\", ""))
        self.assertIn("for(const auto used:usage.capture_used)estimated_events+=used;",web)
        self.assertIn("detail_logs->save_rf(false",main)
        self.assertIn("kPreFilterKey|seq",main)
        self.assertIn("requested_pre_filter",web)
        self.assertIn("export_copy_detail",web)
        # No new special export endpoint or change to the six NVS log limits.
        self.assertIn("SavedLogLimits",cpp)
        self.assertNotIn("capture_used[2]",read("firmware/main/rflink_log_store.hpp").split("struct LogLimits",1)[1].split("};",1)[0])

    def test_native_and_ingress_ui_expose_both_new_rings(self):
        for path in ("firmware/main/webui/app.js",
                     "rflink_builder/static/rflink_web/app.js"):
            script=read(path)
            self.assertIn("r.raw433_capture",script)
            self.assertIn("RAW 433.42 captures",script)
            self.assertIn("RAW 433.92 captures",script)
            self.assertIn("Dedicated 433 MHz capture ring",script)
            self.assertIn("Long RAW candidates (6+ pulses or RMT saturation)",script)


if __name__=="__main__":
    unittest.main()
