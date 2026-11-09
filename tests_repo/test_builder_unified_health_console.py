"""Regression contracts for RFLink quick health and unified ESP-IDF diagnosis.

Host-only tests: no network hardware, flashing or RF validation.
"""
from concurrent.futures import Future
from pathlib import Path
import json
import sys
import tempfile
import threading
import unittest
from types import SimpleNamespace
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "rflink_builder"))
import devices
from devices import DeviceBook
from diagnostic_proxy import console_for_device

PHYSICAL = "rf-123456abcdef"
CONFIG = "cfg-" + "a" * 18
IP = "192.168.178.44"
BOARD = "rflink-s3"
HASH = "a1b2c3d4"


def found(ip=IP, device_id=PHYSICAL):
    return {"id": device_id, "ip": ip, "name": "RFLink",
            "online": True, "board_id": BOARD, "board_hash": HASH,
            "identity_verified": True, "firmware": "0.7.0-exp.11"}


class ImmediatePool:
    def __init__(self, max_workers):
        self.max_workers = max_workers

    def __enter__(self):
        return self

    def __exit__(self, *_):
        return False

    def submit(self, fn, *args, **kwargs):
        future = Future()
        try:
            future.set_result(fn(*args, **kwargs))
        except Exception as exc:
            future.set_exception(exc)
        return future


class HealthTests(unittest.TestCase):
    def create_book(self, folder):
        book = DeviceBook(folder)
        book.devices[CONFIG] = {
            **found(), "id": CONFIG, "managed": True, "configured": True,
            "physical_id": PHYSICAL, "name": "RFLink Garage",
            "key": "private-management-key-123"}
        return book

    def test_scan_does_not_mark_verified_device_offline_before_probe(self):
        with tempfile.TemporaryDirectory() as folder:
            book = self.create_book(folder)
            saw_online = []

            def fake_probe(candidate, **_):
                if candidate == IP:
                    saw_online.append(book.devices[CONFIG]["online"])
                    return found()
                return None

            with patch.object(devices, "ThreadPoolExecutor", ImmediatePool), \
                 patch.object(devices, "probe", side_effect=fake_probe):
                book._scan()

            self.assertEqual(saw_online, [True])
            self.assertTrue(book.devices[CONFIG]["online"])
            self.assertEqual(book.devices[CONFIG]["key"], "private-management-key-123")

    def test_known_device_stays_offline_after_failed_or_mismatched_probe(self):
        with tempfile.TemporaryDirectory() as folder:
            for response in (None, found(device_id="rf-ffffffffffff")):
                book = self.create_book(folder)
                with patch.object(devices, "ThreadPoolExecutor", ImmediatePool), \
                     patch.object(devices, "probe", return_value=response):
                    book._refresh()
                self.assertFalse(book.devices[CONFIG]["online"])
                self.assertEqual(book.devices[CONFIG]["physical_id"], PHYSICAL)
                self.assertEqual(book.devices[CONFIG]["key"], "private-management-key-123")
                self.assertTrue(book.devices[CONFIG].get("last_checked"))

    def test_scan_on_other_hardware_cannot_claim_saved_identity(self):
        with tempfile.TemporaryDirectory() as folder:
            book = self.create_book(folder)
            with patch.object(devices, "ThreadPoolExecutor", ImmediatePool), \
                 patch.object(devices, "probe",
                              side_effect=lambda ip, **_: found(device_id="rf-ffffffffffff")
                              if ip == IP else None):
                book._scan()
            self.assertFalse(book.devices[CONFIG]["online"])
            self.assertEqual(book.devices[CONFIG]["key"], "private-management-key-123")


class Reply:
    status = 200

    def __init__(self, data):
        self.body = json.dumps(data).encode()

    def __enter__(self):
        return self

    def __exit__(self, *_):
        return False

    def read(self, limit):
        return self.body[:limit]


class Opener:
    def __init__(self, *, device_id=PHYSICAL, payload=None):
        self.identity = device_id
        self.payload = payload or {
            "text": "I (50) rflink: radio 0 started\\n",
            "cursor": 35, "boot_id": 987654321, "truncated": False}
        self.requests = []

    def open(self, req, timeout=None):
        self.requests.append(req)
        if req.full_url.endswith("/api/status"):
            return Reply({"board_id": BOARD, "board_hash": HASH,
                          "device_id": self.identity, "firmware": "0.7.0"})
        if "/api/diagnostics/console?" in req.full_url:
            return Reply(self.payload)
        raise AssertionError("Unexpected URL " + req.full_url)


def book(key="private-management-key-123"):
    return SimpleNamespace(
        lock=threading.RLock(),
        devices={CONFIG: {"managed": True, "ip": IP, "physical_id": PHYSICAL,
                          "board_id": BOARD, "board_hash": HASH, "key": key}})


class ConsoleProxyTests(unittest.TestCase):
    def test_only_verified_device_and_correct_key_may_read_console(self):
        opener = Opener()
        with patch("diagnostic_proxy.build_opener", return_value=opener):
            result = console_for_device(book(), CONFIG, 12)
        self.assertEqual(result["cursor"], 35)
        self.assertEqual(result["boot_id"], 987654321)
        self.assertIn("radio 0", result["text"])
        self.assertEqual(len(opener.requests), 2)
        self.assertEqual(opener.requests[0].full_url, "http://" + IP + "/api/status")
        self.assertEqual(opener.requests[1].full_url,
                         "http://" + IP + "/api/diagnostics/console?since=12")
        self.assertEqual(opener.requests[1].get_header("X-rflink-update-key"),
                         "private-management-key-123")
        self.assertIsNone(opener.requests[0].get_header("X-rflink-update-key"))
        self.assertNotIn("private-management-key-123", json.dumps(result))

    def test_swapped_hardware_is_blocked_before_diagnostics_or_secrets(self):
        opener = Opener(device_id="rf-ffffffffffff")
        with patch("diagnostic_proxy.build_opener", return_value=opener):
            with self.assertRaisesRegex(RuntimeError, "Different RFLink"):
                console_for_device(book(), CONFIG)
        self.assertEqual(len(opener.requests), 1)
        self.assertIsNone(opener.requests[0].get_header("X-rflink-update-key"))

    def test_missing_key_or_invalid_cursors_are_rejected(self):
        for cursor in (-1, True, 2**53):
            with self.subTest(cursor=cursor):
                opener = Opener()
                with patch("diagnostic_proxy.build_opener", return_value=opener):
                    with self.assertRaises(ValueError):
                        console_for_device(book(), CONFIG, cursor)
                self.assertEqual(len(opener.requests), 1)
        opener = Opener()
        with patch("diagnostic_proxy.build_opener", return_value=opener):
            with self.assertRaisesRegex(RuntimeError, "management key"):
                console_for_device(book(key=""), CONFIG)
        self.assertEqual(len(opener.requests), 1)

    def test_malformed_or_oversized_response_rejected(self):
        for value in ({"text": "x" * 8193, "cursor": 100, "boot_id": 123},
                      {"text": "ok", "cursor": True, "boot_id": 123},
                      {"text": 12, "cursor": 1, "boot_id": 123},
                      {"text": "ok", "cursor": 1, "boot_id": True}):
            with self.subTest(value=value.get("cursor")):
                with patch("diagnostic_proxy.build_opener",
                           return_value=Opener(payload=value)):
                    with self.assertRaises(ValueError):
                        console_for_device(book(), CONFIG)


class UnifiedUiContract(unittest.TestCase):
    def test_usb_and_ota_use_shared_diagnostic_console_and_old_firmware_fallback(self):
        rf = (ROOT/"rflink_builder/static/rf_console.js").read_text("utf-8")
        flash = (ROOT/"rflink_builder/static/rf_flash.js").read_text("utf-8")
        app = (ROOT/"rflink_builder/static/app.js").read_text("utf-8")
        web = (ROOT/"firmware/main/rflink_web_ota.cpp").read_text("utf-8")
        server = (ROOT/"rflink_builder/server.py").read_text("utf-8")
        self.assertIn("api/diagnostics/device/console?device_id=", rf)
        self.assertIn("api/diagnostics/device/console?device_id=", flash)
        self.assertIn("rfSerialMonitorWithRecovery(port,", flash)
        self.assertIn("rfDiagFetchCrashHistory()", rf)
        self.assertIn("rfFlashReadNetworkDiagnostics", flash)
        self.assertIn("refreshDevices().then(checkKnownDevices)", app)
        self.assertIn("deviceHealthBusy", app)
        self.assertIn("/api/devices/refresh", server)
        self.assertIn('console.uri="/api/diagnostics/console"', web)
        self.assertIn("if(!key_ok(req))", web)
        self.assertIn("previous_log_writer=esp_log_set_vprintf(console_vprintf)", web)


if __name__ == "__main__":
    unittest.main()
