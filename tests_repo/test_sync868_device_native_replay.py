"""Compile real firmware codecs and the ESP32 bounded replay scanner with host g++.

Synthetic captures only. No hardware/OTA/on-air validity is asserted.
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
class OnDeviceSyncReplayTests(unittest.TestCase):
    def test_synthetic_native_scanner_and_real_codecs(self):
        compiler=shutil.which("g++") or shutil.which("clang++")
        with tempfile.TemporaryDirectory() as folder:
            executable=Path(folder)/"native_replay_test"
            args=[
                compiler,"-std=c++17","-O2","-Wall","-Wextra","-Werror",
                "-I"+str(MAIN),"-I"+str(CORE/"include"),
                "-I"+str(HAL/"include"),
                str(FW/"tests/fixture_sync868_device_replay.cpp"),
                str(MAIN/"rflink_sync868_replay.cpp"),
                str(CORE/"src/rflink_itho_codec.cpp"),
                str(CORE/"src/rflink_ramses_codec.cpp"),
                str(CORE/"src/rflink_ramses_tx.cpp"),
                "-o",str(executable)
            ]
            built=subprocess.run(args,capture_output=True,text=True,timeout=80)
            self.assertEqual(built.returncode,0,built.stderr)
            check=subprocess.run([str(executable)],capture_output=True,
                                 text=True,timeout=15)
            self.assertEqual(check.returncode,0,check.stderr)
            self.assertIn("PASS bounded native 868",check.stdout)

    def test_async_replay_uses_existing_protected_capture_export_lease(self):
        web=(MAIN/"rflink_web_ota.cpp").read_text()
        js=(MAIN/"webui/app.js").read_text()
        html=(MAIN/"webui/index.html").read_text()
        cmake=(MAIN/"CMakeLists.txt").read_text()
        assert 'rflink_sync868_replay.cpp' in cmake
        assert 'sync868_replay_task' in web
        assert 'xTaskCreate(sync868_replay_task' in web
        assert 'vTaskDelay(1)' in web
        assert '8192,&worker' in web
        assert 'source_generation' in web
        assert 'begin_evidence_export(&captured)' in web
        assert 'worker->receiver->end_evidence_export()' in web
        assert 'if(!key_ok(req))' in web.split(
            'static esp_err_t sync868_replay_get',1)[1][:150]
        assert 'if(!key_ok(req))' in web.split(
            'static esp_err_t sync868_replay_post',1)[1][:150]
        assert 'replay_get.uri="/api/diagnostics/868-replay"' in web
        assert 'replay_post.uri="/api/diagnostics/868-replay"' in web
        assert "Analyze captured bits" in html
        assert "pollSync868Replay()" in js
        assert "hardware_verified\\\":false" in web or \
               'hardware_verified' in web
        report=web.split('static esp_err_t sync868_replay_get',1)[1].split(
            'static esp_err_t sync_bench_status_get',1)[0]
        assert 'source_id' not in report
        assert 'payload' not in report
        assert 'addr[' not in report
        assert 'hex_msb' not in report


if __name__=="__main__":
    unittest.main()
