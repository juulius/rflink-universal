"""Builder 0.6.40: private, bounded 433/868 Replay Engine integration."""
import json
import os
import pathlib
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"rflink_builder"))
import replay_service


class BuilderReplayUI(unittest.TestCase):
    def test_includes_same_codec_sources_not_a_python_fork(self):
        base=ROOT/"rflink_builder/replay_tools"
        self.assertEqual((ROOT/"tools/rf_replay.py").read_bytes(),
                         (base/"rf_replay.py").read_bytes())
        self.assertEqual((ROOT/"tools/rf_replay_codecs.cpp").read_bytes(),
                         (base/"rf_replay_codecs.cpp").read_bytes())
        docker=(ROOT/"rflink_builder/Dockerfile").read_text()
        self.assertIn("COPY replay_tools/ /app/replay_tools/",docker)
        self.assertIn('ZipFile("/app/default_source.zip")',docker)
        self.assertIn("/app/firmware/components/rflink_decoder_core/src/rflink_ramses_codec.cpp",docker)
        self.assertIn("/app/firmware/components/rflink_decoder_core/src/rflink_itho_codec.cpp",docker)
        self.assertIn("g++ -std=c++17 -O2",docker)
        self.assertIn("RFLINK_RF_REPLAY_CODECS_BIN=/app/replay_tools/rf_replay_codecs",docker)

    def test_433_json_is_analyzed_without_compiler_or_radio(self):
        pulses=[x for i in range(16) for x in
                (320 if i%2==0 else 720,-400)]
        capture={"radio":1,"frequency_hz":433920000,
                 "format":"signed_pulses_us","pulses_us":pulses,"count":32}
        result=replay_service.analyze_bytes(
            json.dumps(capture).encode("utf-8"),"json")
        self.assertEqual(result["schema"],"rflink-offline-replay-v1")
        self.assertEqual(result["capture_count"],1)
        self.assertEqual(result["summary"]["radio_1"],1)
        row=result["results"][0]
        self.assertIsNone(row["result"]["protocol"])
        self.assertEqual(row["result"]["reason"],"candidate_timings_only")
        self.assertFalse(row["result"]["candidates"][0]["protocol_verified"])
        serialized=json.dumps(result)
        self.assertNotIn('"pulses_us":',serialized)
        self.assertNotIn('"hex":',serialized)
        self.assertFalse(result["limits"]["raw_captures_persisted"])
        self.assertEqual(result["limits"]["total_replayable_captures"],1)

    def test_rejects_invalid_size_or_extension(self):
        with self.assertRaises(replay_service.rf_replay.ReplayError):
            replay_service.analyze_bytes(b"","json")
        with self.assertRaises(replay_service.rf_replay.ReplayError):
            replay_service.analyze_bytes(b"hello","zip")
        with self.assertRaises(replay_service.rf_replay.ReplayError):
            replay_service.analyze_bytes(b"*"*(replay_service.MAX_REPLAY_BYTES+1),"csv")
        with self.assertRaises(replay_service.rf_replay.ReplayError):
            replay_service.analyze_bytes(b"{broken","json")
        with self.assertRaises(replay_service.rf_replay.ReplayError):
            replay_service.analyze_bytes(b"hello","csv")

    def test_one_analysis_at_a_time(self):
        replay_service._REPLAY_SLOT.acquire()
        try:
            with self.assertRaises(replay_service.ReplayBusy):
                replay_service.analyze_bytes(b"{}","json")
        finally:
            replay_service._REPLAY_SLOT.release()

    def test_tmp_data_removed_and_results_limited(self):
        sample={"radio":0,"format":"signed_pulses_us",
                "frequency_hz":433420000,"pulses_us":[320,-500],
                "count":2}
        payload=json.dumps([sample]*7).encode("utf-8")
        paths=[]
        load=replay_service.rf_replay.load_captures
        def observing(path):
            paths.append(path)
            self.assertTrue(path.exists())
            return load(path)
        with patch.object(replay_service.rf_replay,"load_captures",side_effect=observing):
            with patch.object(replay_service,"MAX_REPLAY_RESULTS",3):
                result=replay_service.analyze_bytes(payload,"json")
        self.assertEqual(result["capture_count"],3)
        self.assertTrue(result["limits"]["results_truncated"])
        self.assertEqual(result["limits"]["total_replayable_captures"],7)
        self.assertEqual(result["limits"]["analyzed_captures"],3)
        self.assertTrue(paths)
        self.assertFalse(paths[0].exists())

    @unittest.skipUnless(shutil.which("g++") or shutil.which("clang++"),
                         "native C++ compiler needed")
    def test_native_868_unknown_fifo_uses_image_binary_not_runtime_compile(self):
        native=ROOT/"firmware/components/rflink_decoder_core"
        compiler=shutil.which("g++") or shutil.which("clang++")
        with tempfile.TemporaryDirectory() as td:
            binary=pathlib.Path(td)/"native-decoder"
            result=subprocess.run([
                compiler,"-std=c++17","-O2","-I"+str(native/"include"),
                str(ROOT/"tools/rf_replay_codecs.cpp"),
                str(native/"src/rflink_ramses_codec.cpp"),
                str(native/"src/rflink_itho_codec.cpp"),"-o",str(binary)
            ],capture_output=True,text=True,timeout=50)
            self.assertEqual(result.returncode,0,result.stderr)
            sample={"radio":2,"frequency_hz":868300000,
                    "format":"fifo_bytes_hex","count":63,"hex":"FF "*63}
            with patch.dict(os.environ,{"RFLINK_RF_REPLAY_CODECS_BIN":str(binary)}):
                answer=replay_service.analyze_bytes(
                    json.dumps(sample).encode("utf-8"),"json")
            self.assertEqual(answer["results"][0]["result"]["reason"],
                             "legacy_preamble_absent")
            self.assertFalse(answer["results"][0]["result"]["hardware_crc_verified"])
            self.assertEqual(len(answer["decoder_fingerprint"]),16)

    def test_authenticated_ingress_endpoint_and_web_ui_contract(self):
        server=(ROOT/"rflink_builder/server.py").read_text()
        html=(ROOT/"rflink_builder/static/index.html").read_text()
        js=(ROOT/"rflink_builder/static/replay_ui.js").read_text()
        css=(ROOT/"rflink_builder/static/app.css").read_text()
        self.assertIn("if not self._authorized(write=True):",server)
        self.assertIn("'/api/replay/analyze'",server)
        self.assertIn("MAX_REPLAY_BYTES if path == '/api/replay/analyze'",server)
        self.assertIn("X-RFLink-Replay-Type",server)
        self.assertIn("'application/octet-stream'",server)
        self.assertIn("except ReplayBusy as exc",server)
        self.assertIn("except ReplayError as exc",server)
        for markup in ("header-replay","rf-replay-run-a","rf-replay-run-b",
                       "rf-replay-compare","rf-replay-results","rf-replay-radio",
                       "rf-replay-outcome","rf-replay-save-report"):
            self.assertIn('id="'+markup+'"',html)
        self.assertIn('<script src="replay_ui.js" defer></script>',html)
        self.assertIn("'X-RFLink-CSRF':csrf",js)
        self.assertIn("api/replay/analyze",js)
        self.assertIn("textContent",js)
        self.assertIn("capture_id",js)
        self.assertIn("signature(row)",js)
        self.assertNotIn("innerHTML",js)
        self.assertIn(".rf-replay-columns",css)

    def test_version_0640(self):
        b=ROOT/"rflink_builder"
        self.assertIn("version: 0.6.40",(b/"config.yaml").read_text())
        self.assertIn("ARG BUILD_VERSION=0.6.40",(b/"Dockerfile").read_text())
        self.assertIn('v0.6.40</span>',(b/"static/index.html").read_text())
        self.assertIn("## v0.6.40 ",(b/"CHANGELOG.md").read_text())


if __name__=="__main__":
    unittest.main()
