"""Native 433 MHz v0.6.6 checksum-decoder migration: actual compiled C++ tests.

Host tests prove protocol matching, strict band isolation and negative cases.
They do NOT prove reception on the real CC1101 hardware.
"""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
FW=ROOT/"firmware"
CORE=FW/"components/rflink_decoder_core"
RADIO=FW/"components/rflink_radio_hal"
INC=CORE/"include"
SRC=CORE/"src"


class NativeWeather433Rx(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"),"C++17 compiler required")
    def test_actual_native_decoder_with_host_fixtures(self):
        units=[
            SRC/"rflink_native_decoder.cpp",
            SRC/"rflink_user_rx_runtime.cpp",
            SRC/"rflink_user_library.cpp",
            SRC/"rflink_user_vm.cpp",
            SRC/"rflink_itho_codec.cpp",
            SRC/"rflink_ramses_codec.cpp",
            SRC/"rflink_ramses_tx.cpp",
            FW/"tests/test_native_decode.cpp",
        ]
        with tempfile.TemporaryDirectory(prefix="native-433-weather-") as tmp:
            executable=Path(tmp)/"test_native_decode"
            command=[
                "g++","-std=c++17","-O2","-Wall","-Wextra","-Werror",
                "-pthread","-I"+str(INC),
                "-I"+str(RADIO/"include"),"-I"+str(FW/"tests"),
                *map(str,units),"-o",str(executable)
            ]
            build=subprocess.run(command,capture_output=True,text=True,timeout=120)
            self.assertEqual(build.returncode,0,build.stderr)
            ran=subprocess.run([str(executable)],capture_output=True,text=True,timeout=15)
            self.assertEqual(ran.returncode,0,ran.stderr+"\n"+ran.stdout)
            self.assertIn("checksum-protected WH2/Acurite",ran.stdout)

    def test_existing_codes_are_preserved_and_433_sensors_have_own_signal_type(self):
        header=(INC/"rflink_native_decoder.hpp").read_text()
        log=(FW/"main/rflink_log_store.hpp").read_text()
        main=(FW/"main/main.cpp").read_text()
        web=(FW/"main/rflink_web_ota.cpp").read_text()
        native=(SRC/"rflink_native_decoder.cpp").read_text()
        codec=(INC/"rflink_433_weather_rx.hpp").read_text()
        self.assertIn("acurite_609txc, fineoffset_wh2",header)
        self.assertIn("acurite_609txc=7, fineoffset_wh2=8",log)
        for name in ("acurite_609txc","fineoffset_wh2"):
            self.assertIn("EventKind::"+name,main)
            self.assertIn("EventKind::"+name,web)
            self.assertIn("LogCode::"+name,web)
            self.assertIn("dc."+name+"_messages",web)
        self.assertIn("weather433::decode",native)
        self.assertIn("if(outcome==UserRxOutcome::none",native)
        self.assertIn("frequency_hz!=433920000u",codec)
        self.assertIn("crc8_wh2(&b[1],4)!=b[5]",codec)
        self.assertIn("static_cast<uint8_t>(sum)!=b[4]",codec)
        # No generic raw TX path or user decoder metadata exported by this change.
        self.assertNotIn("transmit(",codec)
        self.assertNotIn("esp_",codec)


if __name__=="__main__":
    unittest.main()
