"""Stage 34 ESP-IDF 5.5.6 433 RX filter limit regression.

User's actual v0.7.0-exp.9 UART log: both 433 RMT channels fail at
initial receive arm because 30,000ns > ESP32-S3 RMT source-clock
8-bit threshold (~3,187ns). Does NOT claim the hardware fix is verified.
"""
from pathlib import Path
import subprocess
import unittest

ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/"firmware/components/rflink_radio_hal"
SRC=(BASE/"rflink_rmt_rx.cpp").read_text("utf-8")
HDR=(BASE/"include/rflink_pulse_filter.hpp").read_text("utf-8")

class Stage34RmtFilterTests(unittest.TestCase):
    def test_clock_tree_clamp_is_used_by_every_rmt_arm(self):
        self.assertIn("esp_clk_tree_src_get_freq_hz(",SRC)
        self.assertIn("kMaxHardwareFilterTicks=255u",SRC)
        self.assertIn("std::min<uint64_t>(kRequestedFilterNs,max_ns-1u)",SRC)
        self.assertIn("receive_config_.signal_range_min_ns=hardware_filter_ns_",SRC)
        self.assertIn("constexpr uint32_t kRequestedFilterUs=30",SRC)
        self.assertIn("constexpr uint32_t kIdleNs=5000000",SRC)
        self.assertNotIn("cfg.signal_range_min_ns=kMinPulseNs",SRC)
        self.assertIn("filter_short_pulses_inplace(",SRC)

    def test_original_433_rmt_resources_and_tx_untouched(self):
        self.assertIn("cfg.mem_block_symbols=96",SRC)
        self.assertIn("cfg.flags.with_dma=false",SRC)
        self.assertIn("GPIO_PULLDOWN_ONLY",SRC)
        self.assertIn("cfg.resolution_hz=kResolutionHz",SRC)
        cmake=(BASE/"CMakeLists.txt").read_text("utf-8")
        self.assertIn("esp_hw_support",cmake)
        assert "rflink_rmt_tx.cpp" in cmake
        self.assertTrue((ROOT/"firmware/tests/test_native_rmt_filter.cpp").is_file())

    def test_in_place_pulse_filter_is_bounded_and_not_isr(self):
        self.assertIn("read < count",HDR)
        self.assertIn("pulses[write++]",HDR)
        self.assertIn("next_duration < min_pulse_us",HDR)
        self.assertLess(SRC.index("const esp_err_t armed=self->arm();"),SRC.index("filter_short_pulses_inplace("))
        self.assertNotIn("esphome::",SRC)

if __name__=="__main__":unittest.main()
