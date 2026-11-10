"""Stage 33: distinguish pre-RMT CC1101 failures from RMT and manager 7.

Mocked Python proxy + source contracts only. On-device error stages and RF
reception still require an ESP32-S3 hardware test.
"""
import json
from pathlib import Path
import sys
import threading
import unittest
from types import SimpleNamespace
from unittest.mock import patch

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"rflink_builder"))
from diagnostic_proxy import diagnostics_for_device

DEVICE_ID="cfg-"+"b"*18
PHYSICAL_ID="rf-"+"c"*12
IP="192.168.1.88"

class Reply:
    status=200
    def __init__(self, data):self.data=json.dumps(data).encode("utf-8")
    def __enter__(self):return self
    def __exit__(self,*_):return False
    def read(self,limit):return self.data[:limit]

class FakeOpener:
    def __init__(self,status):self.status=status;self.requested=[]
    def open(self,request,timeout=0):
        self.requested.append(request.full_url)
        if request.full_url=="http://"+IP+"/api/status":return Reply(self.status)
        if request.full_url=="http://"+IP+"/api/logs":return Reply({"events":[]})
        raise AssertionError("unexpected request "+request.full_url)

def book():
    return SimpleNamespace(lock=threading.RLock(),devices={
        DEVICE_ID:{"managed":True,"ip":IP,"board_id":"rflink-s3",
                   "board_hash":"boardhash","physical_id":PHYSICAL_ID,
                   "wifi_password":"do-not-echo"}})

def status():
    return {"board_id":"rflink-s3","board_hash":"boardhash",
      "device_id":PHYSICAL_ID,"firmware":"0.7.0-exp.8",
      "rx43342_start_error_stage":8,
      "rx43342_start_error_code":258,
      "rx43342_start_error_reg":18,
      "rx43342_start_expected":48,
      "rx43342_start_observed":0,
      "rx43342_start_failures":3,
      "rx43392_start_error_stage":9,
      "rx43392_start_error_code":261,
      "rx43392_start_error_reg":-1,
      "rx43392_start_expected":-1,
      "rx43392_start_observed":-1,
      "rx43392_start_failures":3}

class PreRmtStage33Tests(unittest.TestCase):
    def test_native_core_records_every_start_step_not_only_rmt(self):
        source=(ROOT/"firmware/components/rflink_radio_hal/rflink_ook_rx.cpp").read_text()
        header=(ROOT/"firmware/components/rflink_radio_hal/include/rflink_ook_rx.hpp").read_text()
        for name in ("lock","profile","hardware","idle","frequency",
                     "modem_write","modem_read","modem_verify","rmt","enter_rx"):
            self.assertIn(name,header)
        for stage in ("idle","frequency","modem_write","modem_read",
                      "modem_verify","rmt","enter_rx"):
            self.assertIn("OokStartStage::"+stage,source)
        self.assertIn("diagnostics_.register_address=addr",source)
        self.assertIn("diagnostics_.expected=expected",source)
        self.assertIn("diagnostics_.observed=observed",source)
        self.assertIn("std::lock_guard<std::mutex>",source)
        self.assertIn("(void)capture_.stop();",source)
        # User's main flow must not be silently redirected to ESPHome.
        self.assertNotIn("esphome::",source)
        main=(ROOT/"firmware/main/main.cpp").read_text()
        self.assertIn("rflink_set_ook_stats_providers(&ook_a,&ook_b)",main)

    def test_live_proxy_provides_both_radio_stages_and_exact_values(self):
        opener=FakeOpener(status())
        with patch("diagnostic_proxy.build_opener",return_value=opener):
            out=diagnostics_for_device(book(),DEVICE_ID)
        for side in ("rx43342","rx43392"):
            for suffix in ("start_error_stage","start_error_code",
                           "start_error_reg","start_expected","start_observed",
                           "start_failures"):
                field=side+"_"+suffix
                self.assertEqual(out[field],status()[field])
        self.assertEqual(opener.requested,["http://"+IP+"/api/status",
                                           "http://"+IP+"/api/logs"])
        self.assertNotIn("do-not-echo",json.dumps(out))

    def test_invalid_status_data_is_filtered_and_identity_remains_checked(self):
        bad=status()
        for key in list(bad):
            if "start_" in key:bad[key]=True
        opener=FakeOpener(bad)
        with patch("diagnostic_proxy.build_opener",return_value=opener):
            out=diagnostics_for_device(book(),DEVICE_ID)
        self.assertIsNone(out["rx43342_start_error_stage"])
        self.assertIsNone(out["rx43392_start_failures"])
        wrong=status();wrong["device_id"]="rf-"+"d"*12
        opener=FakeOpener(wrong)
        with patch("diagnostic_proxy.build_opener",return_value=opener):
            with self.assertRaises(RuntimeError):
                diagnostics_for_device(book(),DEVICE_ID)
        self.assertEqual(opener.requested,["http://"+IP+"/api/status"])

    def test_copy_and_download_include_first_error_stage(self):
        js=(ROOT/"rflink_builder/static/rf_console.js").read_text()
        web=(ROOT/"firmware/main/rflink_web_ota.cpp").read_text()
        for marker in ("rfDiagCaptureOokFailures(response);","CC1101 RX startup:",
                       "RF_DIAG_OOK_STAGES","rx43342_start_error_reg",
                       "rx43392_start_error_reg",
                       "navigator.clipboard.writeText(rfDiagText)",
                       "new Blob([rfDiagText]"):
            self.assertIn(marker,js)
        for marker in ("rx43342_start_error_stage","rx43392_start_error_stage",
                       "da.register_address","db.register_address"):
            self.assertIn(marker,web)
        self.assertIn("rfDiagCaptureOokFailures(response);",js)
        self.assertIn("const auto da=radio_ready",web)
        self.assertIn("const auto db=radio_ready",web)

if __name__=="__main__":unittest.main()
