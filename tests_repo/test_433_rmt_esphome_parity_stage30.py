"""Stage 30: 433 MHz RMT parity and actionable early-start diagnostics.

The reference settings were verified against the user's v0.6.6 Radio Gaga
rflink-universal.yaml: GDO2 input+pulldown, 96 symbols for each non-DMA
receiver, 30 us filter, 5 ms idle and GPIO 6 / 14. This test does not imply
that the native ESP32-S3 receiver has been validated on physical hardware.
"""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[1]
HAL=ROOT/"firmware/components/rflink_radio_hal"


class Rmt433ParityTests(unittest.TestCase):
    def test_radio_mapping_matches_legacy_433_dual_receiver_pinout(self):
        header=(HAL/"include/rflink_board_generated.hpp").read_text("utf-8")
        self.assertIn("{4,5,6,433420000,Modulation::ook}",header)
        self.assertIn("{9,8,14,433920000,Modulation::ook}",header)
        profiles=(HAL/"rflink_rf_profiles.cpp").read_text("utf-8")
        self.assertIn("{0x00,0x0d}",profiles)  # IOCFG2: async GDO2
        self.assertIn("{0x02,0x2e}",profiles)  # IOCFG0: GDO0 disabled
        self.assertIn("{0x10,0x87}",profiles)  # 203 kHz filter, ~5 kbps symbol rate
        self.assertIn("{0x11,0x93}",profiles)
        self.assertIn("{0x0b,0x06}",profiles)  # 153 kHz IF
        self.assertIn("{0x12,0x30}",profiles)  # ASK/OOK

    def test_gdo2_pulldown_and_non_dma_96_symbol_allocation(self):
        source=(HAL/"rflink_rmt_rx.cpp").read_text("utf-8")
        self.assertIn("cfg.mem_block_symbols=96",source)
        self.assertIn("cfg.flags.with_dma=false",source)
        self.assertIn("gpio_set_pull_mode(pin,GPIO_PULLDOWN_ONLY)",source)
        self.assertLess(source.index("rmt_new_rx_channel(&cfg,&channel_)"),
                        source.index("gpio_set_pull_mode(pin,GPIO_PULLDOWN_ONLY)"))
        # v0.6.6 requested a 30us pulse filter, but ESPHome *clamped*
        # the RMT hardware threshold to the 8-bit 80MHz source clock.
        # Native exp.9's literal 30,000ns hardware threshold caused both
        # 433 RX channels to fail at rmt_receive(ESP_ERR_INVALID_ARG).
        self.assertIn("kRequestedFilterUs=30",source)
        self.assertIn("esp_clk_tree_src_get_freq_hz(",source)
        self.assertIn("receive_config_.signal_range_min_ns=hardware_filter_ns_",source)
        self.assertIn("filter_short_pulses_inplace(",source)
        self.assertIn("kIdleNs=5000000",source)

    def test_first_driver_failure_survives_for_later_wifi_diagnosis(self):
        header=(HAL/"include/rflink_rmt_rx.hpp").read_text("utf-8")
        source=(HAL/"rflink_rmt_rx.cpp").read_text("utf-8")
        web=(ROOT/"firmware/main/rflink_web_ota.cpp").read_text("utf-8")
        self.assertIn("RmtRxFailureStage",header)
        self.assertIn("last_driver_error",header)
        self.assertIn("last_driver_stage",header)
        for name in ("channel","gpio_pull","callbacks","enable","arm","rearm"):
            self.assertIn("RmtRxFailureStage::"+name,source)
        for name in ("rx43342_rmt_error_stage","rx43342_rmt_error_code",
                     "rx43392_rmt_error_stage","rx43392_rmt_error_code"):
            self.assertIn(name,web)
        self.assertIn("rfDiagCaptureOokFailures(response);", (ROOT/"rflink_builder/static/rf_console.js").read_text("utf-8"))
        self.assertIn("esp_err_to_name(e)",source)
        self.assertIn("if(enabled)(void)rmt_disable(channel_)",source)


if __name__=="__main__":
    unittest.main()
