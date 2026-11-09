"""Builder 0.6.22 persistent crash metadata and reconnect UI contracts.

Offline simulations only: no actual serial port, radio or ESP32 crash.
"""
from pathlib import Path
import json
import sys
import threading
import unittest
from types import SimpleNamespace
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "rflink_builder"))
from diagnostic_proxy import crash_report_for_device

DEVICE_ID = "cfg-" + "a" * 18
PHYSICAL_ID = "rf-" + "b" * 12
IP = "192.168.52.67"


class FakeReply:
    status = 200

    def __init__(self, payload):
        self.body = json.dumps(payload).encode("utf-8")

    def __enter__(self):
        return self

    def __exit__(self, *args):
        return False

    def read(self, limit):
        return self.body[:limit]


class FakeOpener:
    def __init__(self, status, crash):
        self.status = status
        self.crash = crash
        self.requests = []

    def open(self, request, timeout=0):
        self.requests.append((request.full_url, timeout))
        if request.full_url == "http://" + IP + "/api/status":
            return FakeReply(self.status)
        if request.full_url == "http://" + IP + "/api/diagnostics/crashes":
            return FakeReply(self.crash)
        raise AssertionError("Unexpected request: " + request.full_url)


def book():
    return SimpleNamespace(
        lock=threading.RLock(),
        devices={
            DEVICE_ID: {
                "managed": True,
                "ip": IP,
                "board_id": "rflink-s3",
                "board_hash": "hash1234",
                "physical_id": PHYSICAL_ID,
                "key": "PRIVATE_OTA_PASSWORD",
                "wifi_password": "PRIVATE_WIFI_PASSWORD",
            },
        },
    )


def status():
    return {"board_id": "rflink-s3", "board_hash": "hash1234",
            "device_id": PHYSICAL_ID, "firmware": "0.7.0-exp.4"}


def crashes():
    return {
        "last_reset": "Task watchdog",
        "history_saved": True,
        "running_firmware": {"version": "0.7.0-exp.4", "elf_sha256": "a"*64,
                             "password": "PRIVATE_WIFI_PASSWORD"},
        "core_dump": {"available": True, "summary_available": True,
                      "integrity": "valid", "build_comparison": "matching_prefix",
                      "elf_sha256": "b"*64, "pc": "0x4037d364", "task": "IDLE0",
                      "stack": "PRIVATE_OTA_PASSWORD"},
        "history": [
            {"boot": i, "reason": "Task watchdog", "reset_code": 7,
             "has_core_dump": i == 16, "pc": "0x4037d364",
             "task": "IDLE0", "wifi_password": "PRIVATE_WIFI_PASSWORD"}
            for i in range(1, 18)
        ],
        "credentials": "PRIVATE_OTA_PASSWORD",
    }


class CrashProxyContractTests(unittest.TestCase):
    def test_identity_checked_and_privacy_bounded(self):
        db = book()
        state_before = json.dumps(db.devices, sort_keys=True)
        fake = FakeOpener(status(), crashes())
        with patch("diagnostic_proxy.build_opener", return_value=fake):
            report = crash_report_for_device(db, DEVICE_ID)
        self.assertEqual([u for u, _ in fake.requests], [
            "http://" + IP + "/api/status",
            "http://" + IP + "/api/diagnostics/crashes",
        ])
        self.assertEqual(report["last_reset"], "Task watchdog")
        self.assertEqual(report["core_dump"]["task"], "IDLE0")
        self.assertEqual(report["core_dump"]["build_comparison"], "matching_prefix")
        self.assertEqual(len(report["history"]), 12)
        self.assertEqual(report["history"][0]["boot"], 1)
        self.assertNotIn("PRIVATE_", json.dumps(report))
        self.assertNotIn("stack", json.dumps(report))
        self.assertNotIn("credentials", json.dumps(report))
        self.assertEqual(json.dumps(db.devices, sort_keys=True), state_before)

    def test_recycled_ip_blocks_crash_request(self):
        db = book()
        for changed in (
            {**status(), "device_id": "rf-" + "f" * 12},
            {**status(), "board_hash": "different"},
        ):
            fake = FakeOpener(changed, crashes())
            with patch("diagnostic_proxy.build_opener", return_value=fake):
                with self.assertRaises(RuntimeError):
                    crash_report_for_device(db, DEVICE_ID)
            self.assertEqual(len(fake.requests), 1)

    def test_unmanaged_and_public_address_never_contacted(self):
        db = book()
        with patch("diagnostic_proxy.build_opener",
                   side_effect=AssertionError("No request expected")):
            db.devices[DEVICE_ID]["managed"] = False
            with self.assertRaises(ValueError):
                crash_report_for_device(db, DEVICE_ID)
            db.devices[DEVICE_ID]["managed"] = True
            db.devices[DEVICE_ID]["ip"] = "8.8.8.8"
            with self.assertRaises(ValueError):
                crash_report_for_device(db, DEVICE_ID)

    def test_size_cap_and_bad_metadata_types(self):
        db = book()
        oversized = crashes()
        oversized["junk"] = "x" * 14000
        fake = FakeOpener(status(), oversized)
        with patch("diagnostic_proxy.build_opener", return_value=fake):
            with self.assertRaisesRegex(RuntimeError, "too large"):
                crash_report_for_device(db, DEVICE_ID)
        malformed = crashes()
        malformed["history"] = [{"boot": True, "reset_code": "abc",
                                 "reason": "reset\u003cscript\u003e",
                                 "has_core_dump": "true", "task": 4}]
        fake = FakeOpener(status(), malformed)
        with patch("diagnostic_proxy.build_opener", return_value=fake):
            report = crash_report_for_device(db, DEVICE_ID)
        self.assertEqual(report["history"][0]["boot"], 0)
        self.assertEqual(report["history"][0]["reset_code"], 0)
        self.assertFalse(report["history"][0]["has_core_dump"])
        self.assertEqual(report["history"][0]["reason"], "resetscript")


class DiagnosisUiContractTests(unittest.TestCase):
    def test_usb_reconnect_and_crash_ui_remain_independent_of_build(self):
        js = (ROOT / "rflink_builder/static/rf_console.js").read_text("utf-8")
        flash = (ROOT / "rflink_builder/static/rf_flash.js").read_text("utf-8")
        html = (ROOT / "rflink_builder/static/index.html").read_text("utf-8")
        backend = (ROOT / "rflink_builder/server.py").read_text("utf-8")
        for marker in (
            "async function rfSerialMonitorWithRecovery(",
            "navigator.serial.getPorts()",
            "matches.length===1",
            "attempts<120",
            "await navigator.serial.requestPort()",
            "api/diagnostics/device/crashes?device_id=",
            "rfDiagFetchCrashHistory()",
        ):
            self.assertIn(marker, js)
        self.assertIn("await rfSerialMonitorWithRecovery(port,", flash)
        self.assertIn("rfFlashWaitForHardware(", flash)
        self.assertIn("rfFlashSerialAfterInstall(port)", flash)
        self.assertIn("rf-diag-last-reset", html)
        self.assertIn("rf-diag-dump-state", html)
        self.assertIn("v0.6.25", html)
        self.assertIn("crash_report_for_device(device_book,device_id)", backend)
        self.assertNotIn("loader.writeFlash(", js)
        self.assertNotIn("api/deploy", js)


if __name__ == "__main__":
    unittest.main()
