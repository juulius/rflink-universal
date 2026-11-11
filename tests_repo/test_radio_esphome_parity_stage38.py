"""Static and numeric guards for the targeted Radio Gaga ESPHome RX compatibility changes.

Host CI cannot validate on-air capture or 868 MHz interrupt latency.
"""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
HAL = ROOT / "firmware/components/rflink_radio_hal"
PROFILES = (HAL / "rflink_rf_profiles.cpp").read_text("utf-8")
RMT = (HAL / "rflink_rmt_rx.cpp").read_text("utf-8")
FILTER = (HAL / "include/rflink_pulse_filter.hpp").read_text("utf-8")
REFERENCE = (ROOT / "reference/v0.6.6-esphome/rflink-universal.yaml").read_text("utf-8")

def itho_registers():
    source = PROFILES.split("constexpr RegisterSetting kItho868[] = {", 1)[1].split("};", 1)[0]
    items = [(int(a, 16), int(b, 16) if b.lower().startswith("0x") else int(b))
             for a, b in re.findall(r"\{(0x[0-9a-fA-F]+)\s*,\s*(0x[0-9a-fA-F]+|[0-9]+)\}", source)]
    result = dict(items)
    if len(result) != len(items):
        raise AssertionError("Duplicate Itho CC1101 register")
    return result

class EspHomeNativeRxParity(unittest.TestCase):
    def test_reference_profile_and_exact_868_modem_math(self):
        self.assertIn("symbol_rate: 38383", REFERENCE)
        self.assertIn("filter_bandwidth: 203kHz", REFERENCE)
        self.assertIn("fsk_deviation: 50kHz", REFERENCE)
        self.assertIn("packet_length: 63", REFERENCE)
        r = itho_registers()
        expected = {
            0x00: 0x0d, 0x02: 0x06, 0x03: 0x0f,
            0x06: 63, 0x07: 0, 0x08: 0,
            0x04: 0xab, 0x05: 0xfe,
            0x0b: 0x06, 0x0c: 0,
            0x10: 0x8a, 0x11: 0x83, 0x12: 0x02,
            0x13: 0x22, 0x14: 0xf8, 0x15: 0x50,
            0x16: 0x07, 0x17: 0x3c, 0x18: 0x14,
            0x19: 0x36, 0x1a: 0x6c,
            0x1b: 0xc7, 0x1c: 0x00, 0x1d: 0xb1,
        }
        self.assertEqual({k: r.get(k) for k in expected}, expected)
        bandwidth = 26_000_000 / (8 * (4 + ((r[0x10] >> 4) & 3)) * (1 << ((r[0x10] >> 6) & 3)))
        deviation = 26_000_000 / (1 << 17) * (8 + (r[0x15] & 7)) * (1 << ((r[0x15] >> 4) & 7))
        self.assertAlmostEqual(bandwidth, 203_125, delta=100)
        self.assertAlmostEqual(deviation, 50_781.25, delta=100)
        self.assertEqual(r[0x17] & 0x0c, 0x0c)  # Continuous native RX preserved
        self.assertEqual(r[0x17] & 0x30, 0x30)  # ESPHome CCA policy
        self.assertIn("rx868_fifo_overflows", (ROOT / "firmware/main/rflink_web_ota.cpp").read_text("utf-8"))

    def test_433_tail_is_bounded_and_worker_only(self):
        self.assertIn("filter: 30us", REFERENCE)
        self.assertIn("idle: 5ms", REFERENCE)
        self.assertIn("rmt_symbols: 96", REFERENCE)
        self.assertIn("append_idle_termination_inplace", FILTER)
        self.assertIn("append_idle_termination_inplace(", RMT)
        self.assertLess(RMT.index("xQueueSend(free_slots_,&event.slot,0)"),
                        RMT.index("append_idle_termination_inplace("))
        self.assertLess(RMT.index("filter_short_pulses_inplace("),
                        RMT.index("append_idle_termination_inplace("))
        isr = RMT.split("bool IRAM_ATTR RmtPulseRx::on_rx_done(", 1)[1].split("esp_err_t IRAM_ATTR RmtPulseRx::arm(", 1)[0]
        self.assertNotIn("append_idle_termination_inplace", isr)
        self.assertIn("kCaptureSlots=3", (HAL / "include/rflink_rmt_rx.hpp").read_text("utf-8"))

if __name__ == "__main__":
    unittest.main()
