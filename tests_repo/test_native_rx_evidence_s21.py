"""Native RX observability and OTA-display regressions.

These are static integration checks; the physical CC1101/RMT and OTA hardware
acceptance tests remain mandatory before treating reception as fixed.
"""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[1]
HAL=ROOT/"firmware/components/rflink_radio_hal"
DECODER=ROOT/"firmware/components/rflink_decoder_core"
MAIN=ROOT/"firmware/main"
RX=(HAL/"rflink_rmt_rx.cpp").read_text()
RX_HEADER=(HAL/"include/rflink_rmt_rx.hpp").read_text()
PROFILE=(HAL/"rflink_rf_profiles.cpp").read_text()
DECODE=(DECODER/"src/rflink_native_decoder.cpp").read_text()
DECODE_HEADER=(DECODER/"include/rflink_native_decoder.hpp").read_text()
API=(MAIN/"rflink_web_ota.cpp").read_text()
APP=(MAIN/"webui/app.js").read_text()
HTML=(MAIN/"webui/index.html").read_text()
CRASH=(MAIN/"rflink_crash_report.cpp").read_text()


class RxEvidenceRegression(unittest.TestCase):
    def test_433_host_buffer_not_rmt_hardware_and_worker_stack(self):
        self.assertIn("cfg.mem_block_symbols=96",RX)
        self.assertIn("static constexpr size_t kMaxSymbols=512",RX_HEADER)
        self.assertIn("rmt_symbol_word_t symbols_[kMaxSymbols]",RX_HEADER)
        self.assertIn("std::array<int32_t,kMaxSymbols*2> scratch_",RX_HEADER)
        self.assertNotIn("std::array<int32_t,kMaxSymbols*2> copy",RX)
        self.assertIn("int32_t* const copy=scratch_.data()",RX)
        self.assertIn("event.symbols<kMaxSymbols",RX)
        self.assertIn("if(complete && n>=2 && sink_)",RX)
        self.assertIn("last_forwarded_pulses",RX)
        self.assertIn("count>1024",DECODE)
        # The decoder does NOT magically decode 433 protocols.
        self.assertIn("EventKind::raw_pulses",DECODE)

    def test_868_records_reasons_without_claiming_crc_or_rf_proof(self):
        for field in ("unknown_prefix_inputs","unknown_decode_inputs"):
            self.assertIn(field,DECODE_HEADER)
            self.assertIn("dc."+field,API)
        self.assertIn("record_event(radio,EventKind::unknown_packet,count,1)",DECODE)
        self.assertIn("record_event(radio,EventKind::unknown_packet,count,2)",DECODE)
        self.assertIn("decode_unknown_prefix",API)
        self.assertIn("decode_unknown_payload",API)
        self.assertIn("rx868_crc_enabled",API)
        self.assertIn("CRC validation is disabled",APP)
        self.assertIn("PKTCTRL0=0x00",PROFILE.replace(" ","").replace("{0x08,0x00}","PKTCTRL0=0x00"))
        self.assertIn("Supported preamble absent",APP)

    def test_http_error_is_actual_http_status_not_enum(self):
        self.assertIn("static int http_status_for_error(httpd_err_code_t code)",API)
        for code,number in (("HTTPD_400_BAD_REQUEST",400),("HTTPD_403_FORBIDDEN",403),
                            ("HTTPD_404_NOT_FOUND",404),("HTTPD_500_INTERNAL_SERVER_ERROR",500)):
            self.assertIn("case "+code+":return "+str(number),API)
        self.assertIn("web_api_failed,http_status_for_error(code)",API)
        self.assertNotIn("web_api_failed,static_cast<int>(code)",API)

    def test_ota_evidence_uses_existing_read_only_nvs_report(self):
        self.assertIn('kOtaHealthKey[] = "ota_health_v1"',CRASH)
        self.assertIn('"ota_health"',CRASH)
        for field in ("diag-ota-version","diag-ota-boot","diag-ota-outcome",
                      "diag-ota-receivers","diag-rx433-summary","diag-rx868-summary"):
            self.assertIn('id="'+field+'"',HTML)
            self.assertIn("'"+field+"'",APP)
        self.assertIn("report.ota_health",APP)
        self.assertIn("Saved in NVS",APP)
        self.assertNotIn("nvs_set_blob",APP)


if __name__=="__main__":
    unittest.main()
