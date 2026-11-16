"""Native 868 FIFO receive instrumentation regression checks.

This module tests the pure counter helper with a real host C++ compiler.
It never claims physical RF validation or changes the CC1101 receive profile.
"""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
HAL=ROOT / "firmware/components/rflink_radio_hal"
MAIN=ROOT / "firmware/main"

class PollEvidence(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++") or shutil.which("clang++"),
                         "C++ host compiler required")
    def test_pure_status_and_gap_counters(self):
        cxx=shutil.which("g++") or shutil.which("clang++")
        with tempfile.TemporaryDirectory() as d:
            binary=Path(d)/"rx868_poll_evidence"
            built=subprocess.run([cxx,"-std=c++17","-Wall","-Wextra",
                                  "-Werror","-O2","-I"+str(HAL/"include"),
                                  str(ROOT/"firmware/tests/test_rx868_poll_telemetry.cpp"),
                                  "-o",str(binary)],
                                 capture_output=True,text=True,timeout=45)
            self.assertEqual(built.returncode,0,built.stderr)
            ran=subprocess.run([str(binary)],capture_output=True,
                               text=True,timeout=15)
            self.assertEqual(ran.returncode,0,ran.stdout+ran.stderr)
            self.assertIn("PASS",ran.stdout)

    def test_driver_api_and_legacy_packet_path_preserved(self):
        h=(HAL/"include/rflink_packet_rx.hpp").read_text()
        c=(HAL/"rflink_packet_rx.cpp").read_text()
        api=(MAIN/"rflink_web_ota.cpp").read_text()
        helper=(HAL/"include/rflink_rx868_poll_telemetry.hpp").read_text()
        assert "Rx868PollTelemetry poll_telemetry" in h
        assert "observe_rx868_poll(" in c
        assert "observe_rx868_status(" in c
        assert "esp_timer_get_time()" in c
        for key in ("rx868_poll_count","rx868_status_valid_polls",
                    "rx868_fifo_partial_polls","rx868_fifo_ready_polls",
                    "rx868_fifo_peak_bytes","rx868_poll_max_gap_us",
                    "rx868_poll_gaps_over_20ms"):
            self.assertIn(key,api)
        # Explicitly disallow SPI/modem commands in the pure monitor:
        for forbidden in ("SFRX","SRX","SIDLE","spi_device_transmit",
                          "bus_.", "gpio_set", "uart_driver"):
            self.assertNotIn(forbidden,helper)
        self.assertIn("bus_.read_rx_fifo(kRadio,packet,sizeof(packet))",c)
        self.assertIn("if((rxbytes&0x7f)>=63)",c)
        self.assertIn("bus_.enter_rx_verified(kRadio)",c)

if __name__ == "__main__":
    unittest.main()
