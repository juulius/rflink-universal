"""Verify the bounded 868-MHz clocked-bit transport using the native codecs.

The test exercises bit-exact, synthetic RAMSES-II UART-on-air encoding and
Itho packet source bytes. It does not configure CC1101 or read a real GPIO.
"""
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
FW=ROOT/"firmware"
HAL=FW/"components/rflink_radio_hal"
CORE=FW/"components/rflink_decoder_core"

class FixedClockedRxEvidenceTest(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++") or shutil.which("clang++"),
                         "host C++ compiler is required")
    def test_native_clocked_bit_capture_and_both_codecs(self):
        compiler=shutil.which("g++") or shutil.which("clang++")
        with tempfile.TemporaryDirectory() as td:
            executable=Path(td)/"rx868_sync_capture"
            command=[compiler,"-std=c++17","-O2","-Wall","-Wextra","-Werror",
                     "-I"+str(HAL/"include"),"-I"+str(CORE/"include"),
                     str(HAL/"rflink_clocked_bit_capture.cpp"),
                     str(CORE/"src/rflink_itho_codec.cpp"),
                     str(CORE/"src/rflink_ramses_codec.cpp"),
                     str(CORE/"src/rflink_ramses_tx.cpp"),
                     str(FW/"tests/test_clocked_bit_capture.cpp"),
                     "-o",str(executable)]
            compile_result=subprocess.run(command,text=True,capture_output=True,
                                          timeout=60)
            self.assertEqual(compile_result.returncode,0,compile_result.stderr)
            run=subprocess.run([str(executable)],text=True,
                               capture_output=True,timeout=20)
            self.assertEqual(run.returncode,0,run.stdout+run.stderr)
            self.assertIn("PASS fixed clocked 868-bit stream",run.stdout)

    def test_live_868_receiver_unmodified_and_sync_rx_not_pretended_active(self):
        packet=(HAL/"rflink_packet_rx.cpp").read_text()
        main=(FW/"main/main.cpp").read_text()
        capture=(HAL/"rflink_clocked_bit_capture.cpp").read_text()
        status=(HAL/"CMakeLists.txt").read_text()
        self.assertIn("bus_.read_rx_fifo(kRadio,packet,sizeof(packet))",packet)
        self.assertIn("if(rx868)rx868->poll_once()",main)
        self.assertIn("rflink_clocked_bit_capture.cpp",status)
        for forbidden in ("write_register(", "bus_.", "gpio_set", "uart_driver",
                          "rmt_receive(", "spi_device", "SFRX"):
            self.assertNotIn(forbidden,capture)

if __name__=="__main__":
    unittest.main()
