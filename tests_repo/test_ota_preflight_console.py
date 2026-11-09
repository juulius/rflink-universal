"""OTA flash-popup console gets an allowlisted live ESP32 snapshot before upload.

Host-only contract: source ordering, formatting and redaction. Does not
claim live OTA/CC1101 hardware validation.
"""
import sys
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'rflink_builder'))
from ota_preflight import format_ota_preflight_status,format_ota_saved_boot_report


class OtaPreflightConsoleTests(unittest.TestCase):
    def test_live_status_is_readable_and_passwords_never_echoed(self):
        status={
            "firmware":"0.7.0-exp.12-n16r8-b5e871b06",
            "release_channel":"experimental",
            "release_build":12,"device_id":"rf-68ee8f479058",
            "board_id":"rflink-s3-n16r8-reva","board_hash":"5e871b06",
            "wifi_connected":True,"mqtt_connected":False,
            "uptime_s":123,"free_heap_bytes":100345,
            "ota_slot":16,"ota_state":2,
            "ota_slot_count":2,"ota_target_slot":17,
            "ota_update_safe":True,"ota_running_address":0x20000,
            "ota_target_address":0x420000,"ota_layout_issue":"ok",
            "radio_initialized":True,"startup_complete":True,
            "spi_bus_ready":True,"spi_clock_hz":6000000,
            "spi_pins":[10,11,12],
            "radio_pins":[[1,2,3],[4,5,6],[7,8,9]],
            "cc1101_present":[True,True,False],
            "spi_ready_timeouts":[0,1,3],
            "rx43342_listening":True,"rx43392_listening":False,
            "rx868_listening":True,
            "rx43342_rmt_error_code":258,
            "rx_worker_cycles":3333,
            "wifi_password":"SUPER_SECRET",
            "update_key":"SECRET_ADMIN_KEY",
            "radio_payload":"PRIVATE RF PAYLOAD",
            "unexpected":"PRIVATE EXTRA",
        }
        output=format_ota_preflight_status(status,"192.168.52.67")
        for part in ("— Live RFLink status", "firmware", "0.7.0-exp.12",
                     "0x20000", "0x420000", "16", "17", "CC1101 detected",
                     "433.42", "868.30", "258", "Wi-Fi: YES", "MQTT: NO"):
            self.assertIn(part.lower(),output.lower())
        for secret in ("SUPER_SECRET","SECRET_ADMIN_KEY",
                       "PRIVATE RF PAYLOAD","PRIVATE EXTRA"):
            self.assertNotIn(secret,output)

    def test_unsupported_fields_are_labelled_unknown_not_invented(self):
        text=format_ota_preflight_status({"firmware":"0.7.0-exp.9"},
                                         "192.168.52.67")
        self.assertIn("Slot count: unknown",text)
        self.assertIn("inactive target: unknown",text)
        self.assertIn("unknown means unsupported",text)
        self.assertIn("safe: unknown",text)

    def test_older_crash_history_and_ota_health_are_allowlisted(self):
        report={
            "last_reset":"Software restart",
            "boot_phases":[{"boot":11,"stage":"app_ready",
                            "startup_complete":True,
                            "raw_stack":"SECRET_STACK"}],
            "ota_health":{"outcome":"failed","firmware":"0.7.0-exp.11",
                          "spi_ok":True,"web_ok":False,"worker_ok":True,
                          "cycles":501,"active_receivers":0,
                          "responsive_radios":2,"secret":"SECRET_KEY"},
            "password":"SUPER_SECRET",
        }
        out=format_ota_saved_boot_report(report)
        self.assertIn("app_ready",out)
        self.assertIn("Previous OTA health: failed",out)
        self.assertIn("active 0",out)
        self.assertNotIn("SUPER_SECRET",out)
        self.assertNotIn("SECRET_STACK",out)
        self.assertNotIn("SECRET_KEY",out)

    def test_preflight_reaches_popup_console_before_upload_or_refusal(self):
        server=(ROOT/"rflink_builder/server.py").read_text()
        popup=(ROOT/"rflink_builder/static/rf_flash.js").read_text()
        ota=server.split("    def _ota(self,params,manifest,release):",1)[1].split(
            "    def _usb(self,params,manifest,release):",1)[0]
        snap=ota.index("log.write(format_ota_preflight_status(status,str(ip)))")
        crash=ota.index("format_ota_saved_boot_report(boot_report)")
        reject=ota.index("if status.get('ota_update_safe') is False:")
        upload=ota.index("result=send_ota_firmware(")
        self.assertLess(snap,crash)
        self.assertLess(crash,reject)
        self.assertLess(reject,upload)
        self.assertIn("format_ota_preflight_status",server)
        self.assertIn("rfFlashDeploymentLog=log",popup)
        self.assertIn("rfFlashComposeHostLog()",popup)
        self.assertIn("rfFlashReplace(",popup)


if __name__=="__main__":
    unittest.main()
