"""Native Radio Gaga boot-guard parity contract.

Reference is the working user-supplied v0.6.6 RadioBootGuard. These
host checks are not a substitute for real ESP-IDF and ESP32 hardware tests.
"""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[1]
FW=ROOT/"firmware"
HAL=FW/"components/rflink_radio_hal"

class NativeRadioGagaBootStage32(unittest.TestCase):
    def test_native_pre_spi_constants_match_radio_gaga_reference(self):
        src=(HAL/"rflink_native_boot_guard.cpp").read_text("utf-8")
        for requirement in (
            "kRailSettleUs=200000", "kChipReadyUs=10000",
            "kReadyRetries=5", "kPostGuardUs=25000",
            "GPIO_MODE_INPUT", "GPIO_FLOATING", "GPIO_PULLUP_ONLY",
            "GPIO_PULLDOWN_ONLY", "gpio_get_level",
        ):
            self.assertIn(requirement,src)
        self.assertIn("prepare_radio_pins_before_spi()",src)
        self.assertNotIn("cc1101::CC1101Component",src)
        self.assertNotIn("esphome::",src)
        self.assertNotIn("spi_device_polling_transmit",src)

    def test_native_boot_order_and_shared_spi_owner_preserved(self):
        main=(FW/"main/main.cpp").read_text("utf-8")
        self.assertLess(main.index("rflink::prepare_radio_pins_before_spi()"),
                        main.index("rflink_start_web_ota();"))
        self.assertLess(main.index("rflink_start_web_ota();"),
                        main.index("e=bus.begin();"))
        self.assertIn("if(!radio_boot_guard.gpio_setup_ok())",main)
        cmake=(HAL/"CMakeLists.txt").read_text("utf-8")
        self.assertIn('"rflink_native_boot_guard.cpp"',cmake)
        host=(FW/"tests/TEST_HOST.sh").read_text("utf-8")
        self.assertIn("tests/test_radiogaga_boot_guard.cpp",host)

    def test_no_native_hybrid_runtime_or_altered_board_pins(self):
        header=(HAL/"include/rflink_board_generated.hpp").read_text("utf-8")
        self.assertIn("SpiPins kSpiPins{12,11,13,200000}",header)
        for values in ("4,5,6,433420000","9,8,14,433920000","15,16,17,868300000"):
            self.assertIn(values,header)
        guide=(ROOT/"docs/NATIVE_RADIOGAGA_PARITY_PORT.md").read_text("utf-8")
        self.assertIn("standalone\nnative ESP-IDF firmware",guide)
        self.assertIn("The **full native port is not complete**",guide)

if __name__=="__main__":
    unittest.main()
