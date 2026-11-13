"""868 MHz FIFO parity: unknown metadata/detail isolation, paging and unchanged modem profile."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]

def source(path):
    return (ROOT/path).read_text(encoding="utf-8")

class Unknown868Parity(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "g++ required")
    def test_real_logstore_868_unknown_storm_isolated_and_pageable(self):
        test=r"""
#include "rflink_log_store.hpp"
#include <cassert>
using namespace rflink;
int main(){
  LogStore store;
  LogLimits limits{};
  limits.noise=5;limits.signal[2]=4;
  assert(store.configure(limits));
  const auto fan=store.push(LogStream::signal,LogCode::ramses,11,2,
                            LogSeverity::info,63,0x22f1);
  const auto sensor=store.push(LogStream::signal,LogCode::raw_pulses,13,0);
  const auto short_433=store.push(LogStream::noise,LogCode::raw_pulses,14,1);
  assert(fan>0 && short_433>0 && sensor>0);
  for(int i=0;i<1900;i++){
    store.push(LogStream::noise,LogCode::unknown_packet,20+i,2,
               LogSeverity::info,63,1);
  }
  auto stats=store.stats();
  assert(stats.unknown868_used==kUnknown868Retention);
  assert(stats.unknown868_overwritten==400);
  assert(stats.used[2]==1 && stats.overwritten[2]==0);
  assert(stats.used[3]==1 && stats.overwritten[3]==0);
  // Existing six metadata rings stay exactly where they were.
  LogRecord rows[64]{};
  LogQuery radio868{};radio868.radio=2;radio868.include_noise=true;
  auto count=store.recent(rows,64,radio868);
  assert(count==64);
  for(size_t i=1;i<count;i++)assert(rows[i].sequence>rows[i-1].sequence);
  LogQuery known{};known.stream=0;known.radio=2;
  count=store.recent(rows,64,known);
  assert(count==1 && rows[0].sequence==fan);
  LogQuery shared{};shared.stream=1;shared.radio=1;
  count=store.recent(rows,64,shared);
  assert(count==1 && rows[0].sequence==short_433);
  LogQuery unknown{};unknown.stream=1;unknown.radio=2;
  const auto cutoff=store.latest_sequence();
  uint64_t cursor=0;size_t total=0;uint64_t previous=0;
  for(int rounds=0;rounds<200;rounds++){
    LogRecord page[17]{};
    const auto n=store.page_after(page,17,cursor,cutoff,unknown);
    if(!n)break;
    for(size_t j=0;j<n;j++){
      assert(page[j].sequence>previous);
      assert(page[j].code==static_cast<uint16_t>(LogCode::unknown_packet));
      previous=page[j].sequence;
      ++total;
    }
    cursor=page[n-1].sequence;
  }
  assert(total==kUnknown868Retention);
  // No new user NVS setting; existing user limits and configured capacities
  // still resize without removing fixed FIFO unknown retention.
  LogLimits resized=limits;resized.noise=2;
  assert(store.configure(resized));
  stats=store.stats();
  assert(stats.unknown868_used==kUnknown868Retention);
  assert(stats.used[3]==1 && stats.used[2]==1);
  assert(stats.allocated_bytes==LogStore::bytes_for(resized));
  return 0;
}"""
        with tempfile.TemporaryDirectory(prefix="rflink-868-unknown-") as tmp:
            cpp=Path(tmp)/"test.cpp";exe=Path(tmp)/"test"
            cpp.write_text(test,encoding="utf-8")
            run=subprocess.run(
                ["g++","-std=c++17","-pthread","-O2","-Wall","-Wextra","-Werror",
                 "-I"+str(ROOT/"firmware/main"),str(cpp),"-o",str(exe)],
                text=True,capture_output=True,timeout=35,
            )
            self.assertEqual(run.returncode,0,run.stderr)
            res=subprocess.run([str(exe)],text=True,capture_output=True,timeout=15)
            self.assertEqual(res.returncode,0,res.stderr)

    def test_quality_is_optional_and_no_hardware_crc_is_claimed(self):
        packet=source("firmware/components/rflink_radio_hal/rflink_packet_rx.cpp")
        decoder=source("firmware/components/rflink_decoder_core/src/rflink_native_decoder.cpp")
        self.assertIn("if((rxbytes&0x80) || (marc&0x1f)==0x11)",packet)
        self.assertIn("if((rxbytes&0x7f)>=63)",packet)
        self.assertIn("read_rx_fifo(kRadio,packet,sizeof(packet))",packet)
        self.assertIn("quality.has_quality=true",packet)
        self.assertIn("this modem profile has CRC disabled",packet)
        self.assertIn("decodePacked(bytes,count,ramses)",decoder)
        self.assertIn("itho::decode_packet(packet,itho_frame)",decoder)
        self.assertIn("unknown_prefix_",decoder)
        self.assertIn("unknown_decode_",decoder)

    def test_metadata_and_exact_fifo_bytes_are_separately_retained(self):
        log=source("firmware/main/rflink_log_store.hpp")
        details=source("firmware/main/rflink_log_details.hpp")
        web=source("firmware/main/rflink_web_ota.cpp")
        self.assertIn("kUnknown868Retention=1500",log)
        self.assertIn("code==LogCode::unknown_packet && radio==2",log)
        self.assertIn("unknown868_overwritten",log)
        self.assertIn("kUnknown868DetailBucket=6",details)
        self.assertIn("kEntryLimits={{250,250,250,2000,512,512,1500}}",details)
        self.assertIn("static_assert(DetailStore::kMaxBudgetBytes*10",details)
        self.assertIn("unknown868_fifo",web)
        self.assertIn("usage.unknown868_used",web)
        self.assertIn("kUnknown868DetailBucket",web)
        self.assertIn("for(const auto used:usage.capture_used)",web)
        self.assertIn("export_copy_detail",web)
        for path in ("firmware/main/webui/app.js",
                     "rflink_builder/static/rflink_web/app.js"):
            ui=source(path)
            self.assertIn("Unknown 868.30 FIFO",ui)
            self.assertIn("r.unknown868_fifo",ui)
            self.assertIn("r.details?.capacity?.[6]",ui)

if __name__=="__main__":
    unittest.main()
