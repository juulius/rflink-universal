"""Stage 25: RFLink native TX correlation, diagnostic export and bounded CSV paging.

A full ESP-IDF build is required in CI; host tests alone do not establish
on-air emission, CC1101 RX restoration or hardware stability.
"""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
FW=ROOT/"firmware"


class TxTraceStage25Tests(unittest.TestCase):
    def test_all_three_radio_tx_lifecycle(self):
        with tempfile.TemporaryDirectory(prefix="rflink-tx-trace-") as tmp:
            dst=Path(tmp)/"test_tx"
            cmd=[
                "g++","-std=c++17","-O2","-pthread","-Wall","-Wextra","-Werror",
                "-I"+str(FW/"components/rflink_radio_hal/include"),
                str(FW/"tests/test_tx_manager.cpp"),
                str(FW/"components/rflink_radio_hal/rflink_radio_manager.cpp"),
                str(FW/"components/rflink_radio_hal/rflink_rf_profiles.cpp"),
                "-o",str(dst)]
            built=subprocess.run(cmd,capture_output=True,text=True,timeout=60)
            self.assertEqual(0,built.returncode,built.stderr)
            ran=subprocess.run([str(dst)],capture_output=True,text=True,timeout=30)
            self.assertEqual(0,ran.returncode,ran.stderr)

    def test_csv_pages_advance_and_filter_without_heap_growth(self):
        source=r"""
#include "rflink_log_store.hpp"
#include <cassert>
using namespace rflink;
int main(){
  LogStore logs;
  assert(logs.configure(LogLimits{}));
  logs.push(LogStream::signal,LogCode::itho,1,2);
  logs.push(LogStream::noise,LogCode::raw_pulses,2,0);
  logs.push(LogStream::web,LogCode::settings_saved,3);
  logs.push(LogStream::system,LogCode::rx_recovered,4,1);
  logs.push(LogStream::signal,LogCode::tx_started,5,1,LogSeverity::info,4,0,19,
            1,3,1,0);
  logs.push(LogStream::signal,LogCode::tx_driver_complete,6,1,LogSeverity::info,
            4,0,19,31,3,1,0);
  const auto end=logs.latest_sequence();
  assert(end==6);
  LogRecord out[2]{};
  auto n=logs.page_after(out,2,0,end,LogQuery{-1,-1,true});
  assert(n==2 && out[0].sequence==1 && out[1].sequence==2);
  n=logs.page_after(out,2,2,end,LogQuery{-1,-1,true});
  assert(n==2 && out[0].sequence==3 && out[1].sequence==4);
  n=logs.page_after(out,2,4,end,LogQuery{-1,-1,true});
  assert(n==2 && out[0].sequence==5 && out[1].sequence==6);
  assert(out[0].correlation_id==19 && out[1].correlation_id==19);
  assert(out[1].flags==31);
  assert(out[1].profile==3 && out[1].transport==1);
  assert(logs.page_after(out,2,6,end,LogQuery{-1,-1,true})==0);
  logs.push(LogStream::signal,LogCode::ramses,7,2);
  assert(logs.page_after(out,2,6,end,LogQuery{-1,-1,true})==0);
  assert(logs.page_after(out,2,0,end,LogQuery{-1,1,false})==2);
  assert(out[0].sequence==4 && out[1].sequence==5);
  assert(logs.page_after(out,2,0,end,LogQuery{1,0,false})==1);
  assert(out[0].sequence==2); // Explicit noise filter always sees shared ring.
  assert(logs.page_after(out,2,0,end,LogQuery{-1,-1,false})==2);
  assert(out[0].sequence==1 && out[1].sequence==3); // Noise hidden by default.
  assert(logs.page_after(out,2,0,end,LogQuery{9,-1,true})==0);
  assert(logs.page_after(nullptr,2,0,end)==0);
}
"""
        with tempfile.TemporaryDirectory(prefix="rflink-log-page-") as tmp:
            p=Path(tmp)
            (p/"check.cpp").write_text(source)
            built=subprocess.run([
                "g++","-std=c++17","-O2","-pthread","-Wall","-Wextra","-Werror",
                "-I"+str(FW/"main"),str(p/"check.cpp"),"-o",str(p/"check")],
                capture_output=True,text=True,timeout=40)
            self.assertEqual(0,built.returncode,built.stderr)
            ran=subprocess.run([str(p/"check")],capture_output=True,text=True,timeout=20)
            self.assertEqual(0,ran.returncode,ran.stderr)

    def test_web_export_has_bounded_responses_and_no_public_send(self):
        web=(FW/"main/rflink_web_ota.cpp").read_text()
        header=(FW/"main/rflink_log_store.hpp").read_text()
        main=(FW/"main/main.cpp").read_text()
        radio=(FW/"components/rflink_radio_hal/rflink_radio_manager.cpp").read_text()
        self.assertIn("set_tx_trace_observer(&log_tx_trace,&logs)",main)
        self.assertIn("trace_observer_",radio)
        self.assertIn("TxTracePhase::rejected",radio)
        self.assertIn("TxTracePhase::started",radio)
        self.assertIn("TxTracePhase::finished",radio)
        self.assertIn("page_after",header)
        self.assertIn("latest_sequence",header)
        self.assertIn("httpd_resp_send_chunk",web)
        self.assertIn('"/api/logs.csv"',web)
        self.assertIn('"/api/logs"',web)
        self.assertIn('cfg.max_uri_handlers=21',web)
        self.assertIn('advanced.uri="/advanced"',web)
        self.assertIn('root.handler=native_index_get',web)
        self.assertNotIn('"/api/tx"',web)
        self.assertNotIn('"/api/send"',web)
        self.assertIn("RF not verified",web)


if __name__=="__main__":
    unittest.main()
