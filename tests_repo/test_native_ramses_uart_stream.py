"""Host-only test of the native RAMSES UART receive stream framer.

No CC1101 access, no ESP-IDF stubs and no live RF transmission.
"""
from __future__ import annotations

import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CORE = ROOT / "firmware" / "components" / "rflink_decoder_core"


class NativeRamsesUartStreamTest(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++") or shutil.which("clang++"),
                         "host C++ compiler is required")
    def test_native_uart_stream_framing(self):
        compiler = shutil.which("g++") or shutil.which("clang++")
        with tempfile.TemporaryDirectory() as tmp:
            binary = Path(tmp) / "ramses_uart_stream_test"
            result = subprocess.run(
                [compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror",
                 "-O2", "-I" + str(CORE / "include"),
                 str(ROOT / "firmware/tests/test_ramses_uart_stream.cpp"),
                 str(CORE / "src/rflink_ramses_uart_stream.cpp"),
                 str(CORE / "src/rflink_ramses_codec.cpp"),
                 str(CORE / "src/rflink_ramses_tx.cpp"),
                 "-o", str(binary)],
                capture_output=True, text=True, timeout=45,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            run = subprocess.run([str(binary)], capture_output=True,
                                 text=True, timeout=15)
            self.assertEqual(run.returncode, 0, run.stdout + run.stderr)
            self.assertIn("PASS", run.stdout)

    def test_port_preserves_bounded_legacy_defaults(self):
        header = (CORE / "include/rflink_ramses_uart_stream.hpp").read_text()
        self.assertIn("kCapacity = 240", header)
        self.assertIn("kIdleTimeoutMs = 120", header)
        self.assertIn("kPendingCapacity = 2", header)
        self.assertIn("void reset()", header)

    def test_transport_remains_unwired_by_default(self):
        # Pure native decoder utility only, not a new device driver.
        stream = (CORE / "src/rflink_ramses_uart_stream.cpp").read_text()
        self.assertNotIn("uart_driver_install", stream)
        self.assertNotIn("gpio_set_", stream)
        self.assertNotIn("bus_.", stream)
        self.assertNotIn("esp_", stream)
        native = (ROOT / "firmware/components/rflink_radio_hal/"
                  "rflink_packet_rx.cpp").read_text()
        self.assertIn("bus_.read_rx_fifo(kRadio,packet,sizeof(packet))", native)
        self.assertIn("rxbytes&0x80", native)


if __name__ == "__main__":
    unittest.main()
