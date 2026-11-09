"""Diagnosis build provenance and per-radio low-level RMT errors (Stage 31).

The running ESP32, never the Builder's selected firmware version, supplies
diagnostic provenance. All tests are local mocks; no radio was powered or
firmware flashed.
"""
from pathlib import Path
import json
import sys
import threading
import unittest
from types import SimpleNamespace
from unittest.mock import patch

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"rflink_builder"))
from diagnostic_proxy import diagnostics_for_device

DEVICE_ID="cfg-"+"a"*18
PHYSICAL="rf-"+"b"*12
IP="192.168.52.67"

class FakeReply:
    status=200
    def __init__(self,payload):self.body=json.dumps(payload).encode()
    def __enter__(self):return self
    def __exit__(self,*_):return False
    def read(self,limit):return self.body[:limit]

class FakeOpener:
    def __init__(self,status):self.status=status;self.requests=[]
    def open(self,request,timeout=0):
        self.requests.append(request.full_url)
        if request.full_url=="http://"+IP+"/api/status":
            return FakeReply(self.status)
        if request.full_url=="http://"+IP+"/api/logs":
            return FakeReply({"events":[{"sequence":1,"uptime_ms":5506,
                      "stream":"system","kind":"rx_restart_failure",
                      "severity":2,"radio":0,"value":7}]})
        raise AssertionError("Unsafe endpoint "+request.full_url)

def book():
    return SimpleNamespace(lock=threading.RLock(),devices={
        DEVICE_ID:{"managed":True,"ip":IP,
            "board_id":"rflink-s3","board_hash":"hash1234",
            "physical_id":PHYSICAL,
            "expected_firmware_version":"0.7.0-exp.6",
            "key":"SENSITIVE_OTA_PASSWORD",
            "wifi_password":"SENSITIVE_WIFI_PASSWORD"}})

def status():
    return {"board_id":"rflink-s3","board_hash":"hash1234",
      "device_id":PHYSICAL,"firmware":"0.7.0-exp.7-n16r8-b5e871b06",
      "firmware_display":"v0.7.0 · Experimental · build 7",
      "release_channel":"Experimental","release_build":7,
      "rx43342_listening":False,"rx43392_listening":False,
      "rx43342_rmt_error_stage":2,"rx43342_rmt_error_code":261,
      "rx43392_rmt_error_stage":6,"rx43392_rmt_error_code":259,
      "radio_initialized":True,"startup_complete":True}

class DiagnosisRunningFirmwareTests(unittest.TestCase):
    def test_live_firmware_wins_over_selected_builder_release(self):
        db=book();fake=FakeOpener(status())
        with patch("diagnostic_proxy.build_opener",return_value=fake):
            out=diagnostics_for_device(db,DEVICE_ID)
        self.assertEqual(out["firmware"],"0.7.0-exp.7-n16r8-b5e871b06")
        self.assertEqual(out["release_channel"],"Experimental")
        self.assertEqual(out["release_build"],7)
        self.assertEqual(out["rx43342_rmt_error_stage"],2)
        self.assertEqual(out["rx43342_rmt_error_code"],261)
        self.assertEqual(out["rx43392_rmt_error_stage"],6)
        self.assertEqual(out["rx43392_rmt_error_code"],259)
        self.assertEqual(out["events"][0]["value"],7)
        self.assertNotIn("SENSITIVE_",json.dumps(out))
        self.assertNotIn("0.7.0-exp.6",json.dumps(out))
        self.assertEqual(fake.requests,["http://"+IP+"/api/status",
                                         "http://"+IP+"/api/logs"])

    def test_old_versions_and_malformed_fields_fail_closed(self):
        broken=status()
        broken.update({"firmware":"0.7.0-exp.7\nPASSWORD=SENSITIVE_OTA_PASSWORD",
          "release_channel":"Experimental\nPRIVATE",
          "release_build":True,
          "rx43342_rmt_error_stage":99,
          "rx43342_rmt_error_code":"261",
          "rx43392_rmt_error_stage":True,
          "rx43392_rmt_error_code":999999})
        with patch("diagnostic_proxy.build_opener",return_value=FakeOpener(broken)):
            out=diagnostics_for_device(book(),DEVICE_ID)
        self.assertNotIn("\n",out["firmware"])
        self.assertNotIn("PASSWORD=",out["firmware"])
        self.assertIsNone(out["release_build"])
        for key in ("rx43342_rmt_error_stage","rx43342_rmt_error_code",
                    "rx43392_rmt_error_stage","rx43392_rmt_error_code"):
            self.assertIsNone(out[key])
        legacy=status()
        for key in list(legacy):
            if key.startswith("rx433") or key.startswith("release_"):
                del legacy[key]
        with patch("diagnostic_proxy.build_opener",return_value=FakeOpener(legacy)):
            out=diagnostics_for_device(book(),DEVICE_ID)
        self.assertIsNone(out["rx43342_rmt_error_stage"])
        self.assertFalse(out["rx43342_listening"])

    def test_identity_rejected_before_logs_even_if_version_looks_right(self):
        mismatch=status();mismatch["device_id"]="rf-"+"c"*12
        fake=FakeOpener(mismatch)
        with patch("diagnostic_proxy.build_opener",return_value=fake):
            with self.assertRaisesRegex(RuntimeError,"Different RFLink"):
                diagnostics_for_device(book(),DEVICE_ID)
        self.assertEqual(fake.requests,["http://"+IP+"/api/status"])

    def test_console_copy_export_and_clear_contain_running_version(self):
        js=(ROOT/"rflink_builder/static/rf_console.js").read_text("utf-8")
        for marker in ("rfDiagCaptureRunningBuild(response)",
           "rfDiagCaptureRmtFailures(response)",
           "Version: ","Channel: ","release_build",
           "Running ESP32 firmware (verified via live Wi-Fi status",
           "RMT failure: ","ESP-IDF error ",
           "rfDiagFirmwareHeader","rfDiagResetText()",
           "navigator.clipboard.writeText(rfDiagText)",
           "new Blob([rfDiagText]"):
            self.assertIn(marker,js)
        self.assertLess(js.index("rfDiagCaptureRunningBuild(response);"),
                        js.index("const events=Array.isArray(response.events)"))
        self.assertIn("rfDiagFirmwareHeader=''; // Never carry firmware identity",js)
        self.assertIn("if(rfDiagSource.value==='wifi'&&rfDiagFirmwareHeader)",js)
        self.assertNotIn("rfDiagDevice.expected_firmware_version",js)
        self.assertIn("firmware build is not verified until the ESP32 reports it",js)

if __name__=="__main__":
    unittest.main()
