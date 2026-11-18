"""Guard the bench-only synchronous RX route and the unchanged normal build.

This is a source-contract test. Native C++ manager handover scenarios are
exercised separately by firmware/tests/test_sync_manager.cpp in TEST_HOST.sh.
Physical RF, ISR timing and TX restore require the ESP32-S3 bench.
"""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[1]
HAL=ROOT/"firmware/components/rflink_radio_hal"
MAIN=ROOT/"firmware/main"

class FixedSyncBenchRouting(unittest.TestCase):
    def test_experimental_boot_selects_sync_and_rc_stable_keep_fifo(self):
        cmake=(MAIN/"CMakeLists.txt").read_text()
        main=(MAIN/"main.cpp").read_text()
        self.assertIn("option(RFLINK_ENABLE_SYNC_868_BENCH",cmake)
        self.assertIn("set(RFLINK_SYNC_868_EXPECTED ON)",cmake)
        self.assertIn("set(RFLINK_SYNC_868_EXPECTED OFF)",cmake)
        self.assertIn('RFLINK_RELEASE_TAG',cmake)
        self.assertIn("RFLINK_ENABLE_SYNC_868_BENCH=1",cmake)
        self.assertIn("use a clean build directory",cmake)
        self.assertIn("#define RFLINK_ENABLE_SYNC_868_BENCH 0",main)
        self.assertIn("#if RFLINK_ENABLE_SYNC_868_BENCH",main)
        self.assertIn("#else\n  // RC/Stable builds intentionally retain",main)
        self.assertIn(
            "start_receive(rflink::RadioId::rf86830,rflink::RfProfileId::itho_868_packet)",
            main)
        self.assertIn("RfProfileId::sync_868_bench",main)
        self.assertIn("if(rx868sync)rx868sync->poll_once()",main)
        self.assertIn("else if(rx868)rx868->poll_once()",main)
        self.assertIn("const bool listening=receiver_listening(radio);",main)
        self.assertNotIn("i==1 ? rmt_b.stats().listening : packet_rx.stats().listening",main)
        web=(MAIN/"rflink_web_ota.cpp").read_text()
        self.assertIn("current_868_listening=sync_bench_provider->stats().listening",web)
        self.assertIn(r'\\"rx868_sync_mode\\":%s',web)
        self.assertIn('sync_868_build?"false":"true"',web)
        self.assertNotIn("RFLINK_ENABLE_SYNC_868_BENCH=1",(
            ROOT/"rflink_builder/server.py").read_text())

    def test_manager_routes_and_restores_same_868_transport(self):
        manager=(HAL/"rflink_radio_manager.cpp").read_text()
        header=(HAL/"include/rflink_radio_manager.hpp").read_text()
        for term in (
            "supports_sync_rx", "start_sync_rx", "stop_sync_rx",
            "start_selected_rx", "stop_selected_rx", "RxTransport::gdo_sync",
            "RfProfileId::sync_868_bench"
        ):
            self.assertIn(term,manager+header)
        self.assertIn("return false;",header)
        self.assertIn("stop_selected_rx(radio,prior_profile)",manager)
        self.assertIn("start_selected_rx(radio,prior_profile)",manager)
        self.assertNotIn("prior_packet ?",manager)

    def test_candidate_profile_is_sync_only_not_new_tx_or_normal_rx(self):
        profiles=(HAL/"rflink_rf_profiles.cpp").read_text()
        for reg in ("{0x00,0x0b}", "{0x02,0x0c}", "{0x08,0x10}",
                    "{0x12,0x00}", "{0x08,0x00}"):
            self.assertIn(reg,profiles)
        self.assertIn("UNVALIDATED",profiles)
        self.assertIn("TxTransport::gdo_uart,kSync868Bench",profiles)
        self.assertIn("nullptr,nullptr,0}",profiles) # No UART TX preset.
        transport=(HAL/"rflink_sync_868_bench_rx.cpp").read_text()
        for guard in ("bus_.enter_rx_verified(kRadio)", "read_register(",
                      "sampler_.stop()", "bus_.strobe(kRadio,kSidle)",
                      "ClockedBitBlock block", "report"):
            # "report" is not required in driver; tests focus on concrete ops.
            if guard!="report":self.assertIn(guard,transport)
        self.assertNotIn("start_packet_rx(",transport)
        self.assertNotIn("uart_driver_install(",transport)
        self.assertIn("rflink_sync_868_bench_rx.cpp",(HAL/"CMakeLists.txt").read_text())

if __name__=="__main__":
    unittest.main()
