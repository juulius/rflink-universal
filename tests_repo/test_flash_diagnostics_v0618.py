"""RFLink Builder 0.6.18 flash / read-only diagnosis regressions.

No real COM port, firmware installation, network connection or password is used.
"""
from contextlib import contextmanager
from html.parser import HTMLParser
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

DEVICE_ID="cfg-"+"a"*18
PHYSICAL="rf-"+"b"*12
IP="192.168.52.67"

class StubReply:
    def __init__(self,data):
        self.data=json.dumps(data).encode()
        self.status=200
    def __enter__(self):return self
    def __exit__(self,*_):pass
    def read(self,size):
        return self.data[:size]

class StubOpener:
    def __init__(self,status,logs):
        self.status=status
        self.logs=logs
        self.requests=[]
    def open(self,req,timeout=0):
        self.requests.append((req.full_url,timeout))
        if req.full_url=="http://"+IP+"/api/status":
            return StubReply(self.status)
        if req.full_url=="http://"+IP+"/api/logs?stream=system":
            return StubReply(self.logs)
        raise AssertionError("Unvalidated URL "+req.full_url)

def saved():
    return {"id":DEVICE_ID,"managed":True,"configured":True,
            "ip":IP,"board_id":"rflink-s3","board_hash":"12345678",
            "physical_id":PHYSICAL,"key":"never-send-this-secret",
            "wifi_password":"never-send-wifi-password"}
def device_book():
    return SimpleNamespace(lock=threading.RLock(),devices={DEVICE_ID:saved()})

class DiagnosisProxyTests(unittest.TestCase):
    def test_reads_known_identity_without_secrets_or_mutations(self):
        book=device_book()
        before=json.dumps(book.devices,sort_keys=True)
        fake=StubOpener(
            {"board_id":"rflink-s3","board_hash":"12345678",
             "device_id":PHYSICAL,"firmware":"0.7.0-exp.2"},
            {"events":[{"sequence":2,"uptime_ms":1250,"stream":"system",
                        "radio":255,"kind":"wifi_connected",
                        "severity":0,"value":0,"wifi_password":"SECRET"}]})
        with patch("diagnostic_proxy.build_opener",return_value=fake):
            result=diagnostics_for_device(book,DEVICE_ID,"system")
        self.assertEqual(len(fake.requests),2)
        self.assertEqual(fake.requests[1][0],"http://"+IP+"/api/logs?stream=system")
        self.assertEqual(result["events"][0]["kind"],"wifi_connected")
        self.assertNotIn("wifi_password",json.dumps(result))
        self.assertNotIn("never-send",json.dumps(fake.requests))
        self.assertEqual(json.dumps(book.devices,sort_keys=True),before)

    def test_blocks_unsaved_device_and_untrusted_ip_without_request(self):
        book=device_book()
        with patch("diagnostic_proxy.build_opener",side_effect=AssertionError("must not call")):
            with self.assertRaises(ValueError):
                diagnostics_for_device(book,"cfg-"+"c"*18)
            with self.assertRaises(ValueError):
                diagnostics_for_device(book,DEVICE_ID,"../settings")
            book.devices[DEVICE_ID]["ip"]="8.8.8.8"
            with self.assertRaises(ValueError):
                diagnostics_for_device(book,DEVICE_ID)
            book.devices[DEVICE_ID]["ip"]=IP
            book.devices[DEVICE_ID]["managed"]=False
            with self.assertRaises(ValueError):
                diagnostics_for_device(book,DEVICE_ID)

    def test_blocks_recycled_ip_and_wrong_physical_id(self):
        book=device_book()
        for changed in (
          {"board_id":"rflink-s3","board_hash":"00000000","device_id":PHYSICAL},
          {"board_id":"rflink-s3","board_hash":"12345678","device_id":"rf-"+"f"*12},
        ):
            fake=StubOpener(changed,{"events":[]})
            with patch("diagnostic_proxy.build_opener",return_value=fake):
                with self.assertRaises(RuntimeError):
                    diagnostics_for_device(book,DEVICE_ID,"system")
            self.assertEqual(len(fake.requests),1,"Do not read logs after identity mismatch")


class Elements(HTMLParser):
    def __init__(self):
        super().__init__()
        self.ids=[];self.scripts=[]
    def handle_starttag(self,tag,attrs):
        attrs=dict(attrs)
        if "id" in attrs:self.ids.append(attrs["id"])
        if tag=="script":self.scripts.append(attrs.get("src",""))

class FlashDiagnosticsUiTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.html=(ROOT/"rflink_builder/static/index.html").read_text("utf-8")
        cls.css=(ROOT/"rflink_builder/static/app.css").read_text("utf-8")
        cls.native=(ROOT/"rflink_builder/static/rf_flash.js").read_text("utf-8")
        cls.diag=(ROOT/"rflink_builder/static/rf_console.js").read_text("utf-8")
        cls.old=(ROOT/"rflink_builder/static/browser_flash.js").read_text("utf-8")
        cls.server=(ROOT/"rflink_builder/server.py").read_text("utf-8")
        cls.doc=Elements();cls.doc.feed(cls.html)

    def test_separate_actions_and_unique_ids(self):
        self.assertEqual(len(self.doc.ids),len(set(self.doc.ids)))
        for element in (
            "detail-diagnostics","rf-diagnostics-dialog","rf-diagnostics-log",
            "rf-diagnostics-source","rf-diagnostics-connect",
            "browser-usb-progress","browser-usb-console","browser-usb-native",
            "browser-usb-stop-monitor","browser-usb-install-button"
        ):self.assertIn(element,self.doc.ids)
        self.assertIn("rf_console.js",self.doc.scripts)
        self.assertIn("rf_flash.js",self.doc.scripts)
        self.assertIn("rfOpenDiagnostics(device)",self.diag)
        self.assertNotIn("api/build",self.diag)
        self.assertNotIn("api/deploy",self.diag)
        self.assertNotIn("api/browser-flash/image",self.diag)

    def test_port_picker_precedes_flash_and_only_after_real_click(self):
        self.assertIn("await navigator.serial.requestPort()",self.native)
        self.assertIn("rfFlashNative.addEventListener('click',rfNativeFlashClick)",self.native)
        self.assertLess(self.native.index("await navigator.serial.requestPort()"),
                        self.native.index("await import('./vendor/esptool-js/bundle.js')"))
        self.assertIn("eraseAll:true",self.native)
        self.assertIn("loader.writeFlash(",self.native)
        self.assertIn("reportProgress(",self.native)
        self.assertIn("loader.after('hard_reset')",self.native)
        self.assertIn("rfFlashSerialAfterInstall(port)",self.native)
        self.assertIn("browserUsbLibraryLoaded().then(",self.old,
                      "Native flash must not be blocked on legacy installer readiness")

    def test_local_vendor_and_single_console_for_all_methods(self):
        docker=(ROOT/"rflink_builder/Dockerfile").read_text("utf-8")
        self.assertIn("esptool-js@0.7.1/bundle.js",docker)
        self.assertIn("'/vendor/esptool-js/bundle.js'",self.server)
        self.assertIn("default-src 'self'",self.server)
        self.assertIn("connect-src 'self' blob:",self.server)
        flow=(ROOT/"rflink_builder/static/device_firmware.js").read_text("utf-8")
        self.assertIn("rfFlashHostStart(method,d)",flow)
        self.assertIn("rfFlashHostUpdate(dep,output.text||'')",flow)
        self.assertIn("rfFlashOpen('browser',d.name)",self.old)
        self.assertIn("rf-console-dialog",self.css)
        self.assertIn("rf-terminal",self.css)
        self.assertIn("/api/diagnostics/device",self.server)
        self.assertIn("v0.6.26",self.html)

if __name__=="__main__":
    unittest.main()
