"""Physical-ID-first DHCP rediscovery regression: LAN fixtures only.

The builder must never replace a managed RFLink identity with a different
ESP32 simply because it is reachable on the former IP or shares a pinout.
"""
from concurrent.futures import Future
from pathlib import Path
from unittest.mock import patch
import sys
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"rflink_builder"))
import devices
from devices import DeviceBook

ID="rf-112233445566"
WRONG="rf-998877665544"
BOARD="rflink-s3-n16r8-reva"
HASH="5e871b06"
OLD="192.168.52.67"
NEW="192.168.52.99"


class ImmediatePool:
    opened=[]
    def __init__(self,max_workers):
        self.opened.append(max_workers)
    def __enter__(self):return self
    def __exit__(self,*args):return False
    def submit(self,fn,*args,**kwargs):
        future=Future()
        try:future.set_result(fn(*args,**kwargs))
        except Exception as exc:future.set_exception(exc)
        return future


def radio(ip,ident=ID,board_hash=HASH):
    return dict(id=ident,ip=ip,name="Discovered RFLink",
                board_id=BOARD,board_hash=board_hash,firmware="0.7.0-exp.40",
                managed=False,online=True,identity_verified=True,
                wifi_connected=True,startup_complete=True)


class IpIdentityRecovery(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.book=DeviceBook(self.tmp.name)
        self.key="private-ota-key-never-to-share"
        self.cfg="cfg-device-unique"
        self.book.devices[self.cfg]={
            "id":self.cfg,"name":"RFLink Ventilation","managed":True,
            "configured":True,"provisioned":True,
            "physical_id":ID,"identity_verified":True,
            "board_id":BOARD,"board_hash":HASH,"firmware":"0.7.0-exp.39",
            "ip":OLD,"online":False,"key":self.key,
            "wifi_ssid":"test-iot","wifi_password":"private-wifi-secret",
            "board":{"spi":{"clock_hz":200000}}}
        self.book._persist()

    def run_scan(self, responses):
        checked=[]
        def fake_probe(ip,**kwargs):
            checked.append(ip)
            return responses.get(ip)
        with patch.object(devices,"ThreadPoolExecutor",ImmediatePool), \
             patch.object(devices,"probe",side_effect=fake_probe):
            self.book._recover()
        return checked

    def test_dhcp_change_recovers_exact_device_and_persists_after_restart(self):
        self.book.set_network("192.168.178.0/24")
        checked=self.run_scan({NEW:radio(NEW),
                               "192.168.178.25":radio("192.168.178.25",WRONG)})
        saved=self.book.devices[self.cfg]
        self.assertIn(NEW,checked)
        self.assertIn("192.168.178.25",checked)
        self.assertTrue(saved["online"])
        self.assertEqual(saved["ip"],NEW)
        self.assertEqual(saved["previous_ip"],OLD)
        self.assertTrue(saved["ip_updated_at"])
        self.assertEqual(saved["physical_id"],ID)
        self.assertEqual(saved["key"],self.key)
        self.assertEqual(saved["name"],"RFLink Ventilation")
        self.assertEqual(saved["board"]["spi"]["clock_hz"],200000)
        self.assertEqual(len([x for x in self.book.devices.values() if x.get("managed")]),1)
        self.assertNotIn(WRONG,self.book.devices)
        self.assertEqual(self.book.status()["devices"][0]["has_key"],True)
        self.assertNotIn(self.key,str(self.book.status()))
        restored=DeviceBook(self.tmp.name)
        self.assertEqual(restored.devices[self.cfg]["ip"],NEW)
        self.assertFalse(restored.devices[self.cfg]["online"],
                         "A saved IP is never proof of online status after a restart")
        self.assertEqual(restored.devices[self.cfg]["key"],self.key)

    def test_wrong_board_hash_or_wrong_mac_cannot_rebind(self):
        self.run_scan({NEW:radio(NEW,ID,"bad-hash"),
                       "192.168.52.70":radio("192.168.52.70",WRONG)})
        self.assertFalse(self.book.devices[self.cfg]["online"])
        self.assertEqual(self.book.devices[self.cfg]["ip"],OLD)
        self.assertEqual(self.book.devices[self.cfg]["key"],self.key)
        self.assertEqual(len(self.book.devices),1)

    def test_duplicate_same_hardware_id_is_refused(self):
        self.run_scan({NEW:radio(NEW),
                       "192.168.52.100":radio("192.168.52.100")})
        saved=self.book.devices[self.cfg]
        self.assertFalse(saved["online"])
        self.assertEqual(saved["ip"],OLD)
        self.assertEqual(saved["discovery_hint"],"duplicate_identity")

    def test_explicit_configured_vlan_recovers_device_after_subnet_move(self):
        self.book.set_network("10.23.45.0/24")
        new_ip="10.23.45.199"
        checked=self.run_scan({new_ip:radio(new_ip)})
        self.assertIn(new_ip,checked)
        self.assertIn(OLD,checked)
        self.assertTrue(self.book.devices[self.cfg]["online"])
        self.assertEqual(self.book.devices[self.cfg]["ip"],new_ip)

    def test_old_ip_reassigned_to_different_rflink_never_becomes_online(self):
        with patch.object(devices,"ThreadPoolExecutor",ImmediatePool), \
             patch.object(devices,"probe",return_value=radio(OLD,WRONG)):
            self.book._refresh()
        self.assertFalse(self.book.devices[self.cfg]["online"])
        self.assertEqual(self.book.devices[self.cfg]["key"],self.key)
        self.assertEqual(self.book.devices[self.cfg]["discovery_hint"],"identity_mismatch")
        self.run_scan({OLD:radio(OLD,WRONG),NEW:radio(NEW)})
        self.assertEqual(self.book.devices[self.cfg]["ip"],NEW)
        self.assertTrue(self.book.devices[self.cfg]["online"])

    def test_legacy_ip_only_device_must_not_auto_migrate(self):
        self.book.devices[self.cfg]["physical_id"]="ip:"+OLD
        self.book.devices[self.cfg]["identity_verified"]=False
        checked=self.run_scan({NEW:radio(NEW)})
        self.assertEqual(checked,[])
        self.assertFalse(self.book.devices[self.cfg]["online"])
        self.assertEqual(self.book.devices[self.cfg]["ip"],OLD)

    def test_explicit_add_new_ip_updates_existing_config_without_new_card(self):
        with patch.object(devices,"probe",return_value=radio(NEW)):
            result=self.book.add(NEW)
        self.assertEqual(len(result["devices"]),1)
        self.assertEqual(self.book.devices[self.cfg]["ip"],NEW)
        self.assertEqual(self.book.devices[self.cfg]["key"],self.key)

    def test_manual_discovery_includes_last_verified_iot_subnet(self):
        checked=[]
        with patch.object(devices,"ThreadPoolExecutor",ImmediatePool), \
             patch.object(devices,"probe",side_effect=lambda ip,**kw: checked.append(ip) or None):
            self.book._scan()
        self.assertIn(NEW,checked)
        self.assertIn("192.168.178.123",checked)
        self.assertLessEqual(len(checked),3*254)
        self.assertTrue(all(n <= 12 for n in ImmediatePool.opened))

    def test_recovery_scheduler_throttles_and_avoids_overlapping_scans(self):
        with patch.object(devices.threading,"Thread") as thread, \
             patch.object(devices.time,"monotonic",side_effect=[1000,1010,1310]):
            self.assertTrue(self.book.recover_offline())
            self.assertFalse(self.book.recover_offline())
            self.book.scanning=False
            self.book.recovering=False
            self.assertFalse(self.book.recover_offline())
            self.assertFalse(self.book.recovering)
            # Retry after five minutes, with a fresh bounded worker.
            self.assertTrue(self.book.recover_offline())
        self.assertEqual(thread.call_count,2)

    def test_background_loop_is_not_unbounded_full_discovery(self):
        server=(ROOT/"rflink_builder/server.py").read_text()
        self.assertIn("device_book.recover_offline()",server)
        self.assertIn("RECOVERY_MAX_SUBNETS = 2",
                      (ROOT/"rflink_builder/devices.py").read_text())
        self.assertIn("RECOVERY_INTERVAL_SECONDS = 300",
                      (ROOT/"rflink_builder/devices.py").read_text())


if __name__=="__main__":unittest.main()
