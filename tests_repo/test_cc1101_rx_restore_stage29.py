"""Stage 29: source-level contract for CC1101 restore hardening.

The real SPI/MARCSTATE/PLL transition needs ESP32-S3 hardware. These checks
guard the integration with the native radio manager; they are not substitutes
for over-the-air TX/RX or PLL tests on a board.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
HAL = ROOT / "firmware/components/rflink_radio_hal"


class VerifiedReceiveRestoreTests(unittest.TestCase):
    def test_bounded_state_and_pll_verification_follows_idle_flush_rx(self):
        source = (HAL / "rflink_cc1101_spi.cpp").read_text("utf-8")
        method = source.split("esp_err_t Cc1101SpiBus::enter_rx_verified", 1)[1].split(
            "ProbeResult Cc1101SpiBus::boot_reset_and_probe", 1
        )[0]
        for expected in (
            "kStateWaitUs=100000", "kPllRetries=3",
            "esp_timer_get_time()", "read_runtime_status(id,&marc,&bytes)",
            "kIdleState=0x01", "kRxState=0x0d",
            "kFscal1=0x25", "kPllNotLocked=0x3f",
            "ESP_ERR_TIMEOUT", "ESP_ERR_INVALID_RESPONSE",
        ):
            self.assertIn(expected, method)
        self.assertLess(method.index("strobe(id,kSidle)"),
                        method.index("wait_for(kIdleState,&last)"))
        self.assertLess(method.index("wait_for(kIdleState,&last)"),
                        method.index("strobe(id,kSfrx)"))
        self.assertLess(method.index("strobe(id,kSfrx)"),
                        method.index("strobe(id,kSrx)"))
        self.assertLess(method.index("strobe(id,kSrx)"),
                        method.index("wait_for(kRxState,&last)"))
        # Runtime recovery must not reset the entire chip or the SPI bus.
        self.assertNotIn("kSres", method)
        self.assertNotIn("boot_reset_and_probe(id)", method)

    def test_verified_rearm_is_shared_by_all_three_receivers(self):
        packet = (HAL / "rflink_packet_rx.cpp").read_text("utf-8")
        pulse = (HAL / "rflink_ook_rx.cpp").read_text("utf-8")
        self.assertGreaterEqual(packet.count("bus_.enter_rx_verified(kRadio)"), 2)
        self.assertIn("bus_.enter_rx_verified(radio_)", pulse)
        # Never set 'listening' true on an unchecked SRX SPI acknowledgement.
        self.assertIn("stats_.listening=(e==ESP_OK)", packet)
        self.assertNotIn("bus_.strobe(radio_,kSrx)", pulse)

    def test_uart_releases_gdo_and_explains_restore_failure(self):
        uart = (HAL / "rflink_uart_tx.cpp").read_text("utf-8")
        self.assertIn("uart_driver_delete(UART_NUM_1)", uart)
        self.assertIn("gpio_reset_pin(static_cast<gpio_num_t>(pin))", uart)
        self.assertIn("gpio_set_direction(static_cast<gpio_num_t>(pin),GPIO_MODE_INPUT)", uart)
        self.assertLess(uart.index("uart_driver_delete(UART_NUM_1)"),
                        uart.index("gpio_set_direction(static_cast<gpio_num_t>(pin),GPIO_MODE_INPUT)"))
        self.assertIn("TX restore register 0x%02x mismatch", uart)
        self.assertIn("TX cleanup UART1 release failed", uart)
        self.assertIn("return {false,false,TxFailureStage::restore}", uart)

    def test_sticky_manager_fault_remains_on_unverified_rx(self):
        manager = (HAL / "rflink_radio_manager.cpp").read_text("utf-8")
        self.assertIn("if(!outcome.hardware_restored)", manager)
        self.assertIn("st.operation=RadioOperation::fault;", manager)
        self.assertIn("st.last_error=ManagerError::restore_failure", manager)
        # A TX request is never accepted while the radio is faulted.
        self.assertIn("if(st.operation!=RadioOperation::idle && st.operation!=RadioOperation::receiving)", manager)


if __name__ == "__main__":
    unittest.main()
