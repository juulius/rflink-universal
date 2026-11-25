"""Native experimental 868 synchronous live logging regression.

Synthetic real-codec bitstreams on host only: not physical RF validation.
"""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
FW=ROOT/"firmware"
CORE=FW/"components/rflink_decoder_core"
MAIN=FW/"main"
HAL=FW/"components/rflink_radio_hal"


@unittest.skipUnless(shutil.which("g++") or shutil.which("clang++"),
                     "C++17 host compiler required")
class LiveSync868Tests(unittest.TestCase):
    def test_native_live_uses_same_itho_and_ramses_codec_as_replay(self):
        compiler=shutil.which("g++") or shutil.which("clang++")
        with tempfile.TemporaryDirectory() as folder:
            binary=Path(folder)/"native_sync868_live"
            sources=[
                FW/"tests/fixture_sync868_live_decode.cpp",
                MAIN/"rflink_sync868_live.cpp",
                MAIN/"rflink_sync868_replay.cpp",
                CORE/"src/rflink_itho_codec.cpp",
                CORE/"src/rflink_ramses_codec.cpp",
                CORE/"src/rflink_ramses_tx.cpp",
            ]
            args=[compiler,"-std=c++17","-O2","-Wall","-Wextra","-Werror",
                  "-I"+str(MAIN),"-I"+str(CORE/"include"),
                  "-I"+str(HAL/"include"),
                  *map(str,sources),"-o",str(binary)]
            built=subprocess.run(args,capture_output=True,text=True,timeout=90)
            self.assertEqual(built.returncode,0,built.stderr)
            ran=subprocess.run([str(binary)],capture_output=True,text=True,timeout=15)
            self.assertEqual(ran.returncode,0,ran.stderr)
            self.assertIn("PASS sync868 live codec",ran.stdout)

    def test_nonblocking_queue_and_single_receiver_owner(self):
        main=(MAIN/"main.cpp").read_text()
        receiver=(HAL/"rflink_sync_868_bench_rx.cpp").read_text()
        hdr=(HAL/"include/rflink_sync_868_bench_rx.hpp").read_text()
        web=(MAIN/"rflink_web_ota.cpp").read_text()
        store=(MAIN/"rflink_log_store.hpp").read_text()
        cmake=(MAIN/"CMakeLists.txt").read_text()
        self.assertIn("rflink_sync868_live.cpp",cmake)
        self.assertIn("xQueueSend(sync868_queue,&work,0)",main)
        self.assertIn("xQueueReceive(sync868_queue,&work,portMAX_DELAY)",main)
        self.assertIn("set_block_observer(&forward_sync868_block",main)
        self.assertIn("if(block_observer_)block_observer_",receiver)
        self.assertIn("BlockObserver block_observer_",hdr)
        self.assertIn("rx868_sync_itho_candidate",store)
        self.assertIn("rx868_sync_ramses_valid",store)
        self.assertIn("rx868_sync_activity",store)
        self.assertIn("rx868_sync_queue_loss",store)
        self.assertIn("rx868_sync_edges",web)
        self.assertIn("rx868_sync_blocks",web)
        self.assertIn("sync868_live.consume",main)
        self.assertIn("replay_sync868_frozen(", (MAIN/"rflink_sync868_live.cpp").read_text())
        # Do not accidentally activate the old FIFO packet driver together
        # with the sync GPIO clocked transport.
        self.assertIn("#if RFLINK_ENABLE_SYNC_868_BENCH",main)
        self.assertIn("rx868sync=&bench_sync_rx;",main)
        self.assertIn("else if(rx868)rx868->poll_once();",main)

    def test_live_radio_log_is_not_falsely_counted_as_fifo_packets(self):
        for path in (MAIN/"webui/app.js",
                     ROOT/"rflink_builder/static/rflink_web/app.js"):
            js=path.read_text()
            self.assertIn("s.rx868_sync_mode===true",js)
            self.assertIn("s.rx868_sync_blocks",js)
            self.assertIn("if(e.code===51)",js)
            self.assertIn("if(e.code===52 || e.code===53)",js)
            self.assertIn("if(e.code===54)",js)
            self.assertIn("RX bit blocks (not packets)",js)
        main=(MAIN/"main.cpp").read_text()
        # Routine bit activity must never consume any retained log ring.
        self.assertNotIn("rflink::LogCode::rx868_sync_activity",main)
        # Check the actual periodic RX worker block, not unrelated warnings.
        worker=main.split("void radio_rx_task(void*) {",1)[1]
        periodic=worker.split("if(rx868sync &&",1)[1].split("#endif",1)[0]
        self.assertIn("if(event_logs && new_drops)",periodic)
        self.assertIn("event_logs->push(rflink::LogStream::system,",periodic)
        self.assertIn("rflink::LogCode::rx868_sync_queue_loss",periodic)
        self.assertIn("rflink::LogSeverity::warning",periodic)
        self.assertEqual(periodic.count("event_logs->push("),1)
        self.assertIn('if(new_drops)ESP_LOGW("rflink.sync868",',periodic)
        self.assertNotIn("ESP_LOGI(",periodic)
        for path in (MAIN/"webui/app.js",
                     ROOT/"rflink_builder/static/rflink_web/app.js"):
            js=path.read_text()
            self.assertIn("const syncView=radioView&&selected==='2'",js)
            self.assertIn("node.hidden=!(inspect||syncView)",js)
            self.assertIn("node.textContent=$('diag-rx868-summary').textContent",js)
            self.assertNotIn("raw bits are 30-second Radio diagnostics",js)
        self.assertIn("LogCode::rx868_sync_queue_loss",
                      (MAIN/"rflink_log_store.hpp").read_text())

if __name__=="__main__":
    unittest.main()
