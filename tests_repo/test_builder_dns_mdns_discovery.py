"""RFLink DNS and unpaired DNS-SD regression tests (no real LAN)."""
import json
from concurrent.futures import Future
from pathlib import Path
import socket
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"rflink_builder"))
from identity_discovery import (
    dns_domain, physical_hostname, safe_dns_ip,
    dns_candidates, find_verified_dns,
)
from devices import DeviceBook
import devices
import discovery_announcements as annonces

ID="rf-68ee8f479058"
WRONG="rf-aaaaaaaaaaaa"
BOARD="rflink-s3-n16r8-reva"
HASH="5e871b06"
OLD="192.168.52.67"
NEW="192.168.52.155"


class ImmediatePool:
    def __init__(self,max_workers):
        self.workers=max_workers
    def __enter__(self):
        return self
    def __exit__(self,*args):
        return False
    def submit(self,fn,*args,**kwargs):
        future=Future()
        try:
            future.set_result(fn(*args,**kwargs))
        except Exception as error:
            future.set_exception(error)
        return future


def found(ip=NEW,ident=ID,board_hash=HASH):
    return dict(id=ident,ip=ip,name="RFLink "+ip,online=True,
                identity_verified=True,managed=False,board_id=BOARD,
                board_hash=board_hash,firmware="0.7.0-exp.60",
                wifi_connected=True,startup_complete=True)


def answer(ip=NEW):
    return [(socket.AF_INET,socket.SOCK_STREAM,6,"",(ip,80))]


class DomainTests(unittest.TestCase):
    def test_unique_mac_name_not_based_on_dhcp_or_builder_profile(self):
        self.assertEqual(physical_hostname(ID),"rflink-68ee8f479058")
        for invalid in ("rf-foobar","ip:192.168.52.67",""):
            self.assertEqual(physical_hostname(invalid),"")

    def test_dns_validation_and_no_public_dns_pivots(self):
        for candidate in ("HOME.ARPA.","lan","iot.home.arpa",""):
            self.assertEqual(dns_domain(candidate),candidate.rstrip(".").lower())
        for bad in ("evil.local","a..b","-bad.example","x"*64+".lan",
                    "bad/name","192.168.1.1:80"):
            with self.subTest(bad=bad),self.assertRaises(ValueError):
                dns_domain(bad)
        for blocked in ("8.8.8.8","127.0.0.1","169.254.169.254",
                        "224.0.0.251","0.0.0.0","::1"):
            self.assertIsNone(safe_dns_ip(blocked))
        for good in ("192.168.52.67","10.3.5.6","172.16.0.11"):
            self.assertEqual(safe_dns_ip(good),good)

    def test_only_fixed_hardware_name_resolved_and_public_addresses_discarded(self):
        called=[]
        def resolver(name,port,family,kind):
            called.append((name,port,family,kind))
            return answer(NEW)+answer("8.8.8.8")+answer(NEW)
        self.assertEqual(
            dns_candidates(ID,"home.arpa",resolver=resolver),[NEW])
        self.assertEqual(called,[("rflink-68ee8f479058.home.arpa",
                                  80,socket.AF_INET,socket.SOCK_STREAM)])

    def test_dns_lookup_and_http_board_mismatch_are_never_accepted(self):
        with patch("identity_discovery.socket.getaddrinfo",return_value=answer()):
            self.assertIsNone(find_verified_dns(
                ID,BOARD,HASH,"home.arpa",lambda ip,**kw: found(ip,WRONG)))
            self.assertIsNone(find_verified_dns(
                ID,BOARD,HASH,"home.arpa",
                lambda ip,**kw: found(ip,ID,"wrong-board-hash")))
            self.assertEqual(find_verified_dns(
                ID,BOARD,HASH,"home.arpa",lambda ip,**kw: found(ip))["ip"],NEW)

    def test_dns_multiple_claimants_same_mac_are_ambiguous(self):
        with patch("identity_discovery.socket.getaddrinfo",
                   return_value=answer(NEW)+answer("192.168.52.156")):
            self.assertIsNone(find_verified_dns(
                ID,BOARD,HASH,"home.arpa",lambda ip,**kw: found(ip)))


class BookDnsTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.book=DeviceBook(self.temp.name)
        self.cfg="cfg-device-id"
        self.book.devices[self.cfg]={
            "id":self.cfg,"managed":True,"configured":True,"provisioned":True,
            "physical_id":ID,"identity_verified":True,
            "ip":OLD,"board_id":BOARD,"board_hash":HASH,
            "name":"RFLink Fan","online":False,"key":"secret-key-existing"}
        self.book.set_network("192.168.52.0/24","home.arpa")

    def test_suffix_persists_without_network_override(self):
        self.book.set_network("","LAN")
        reopened=DeviceBook(self.temp.name)
        self.assertEqual(reopened.network,"")
        self.assertEqual(reopened.dns_domain,"lan")
        self.assertEqual(reopened.devices[self.cfg]["key"],"secret-key-existing")

    def test_health_reconnects_ip_via_dns_without_subnet_scan(self):
        seen=[]
        def fake_probe(ip,**kwargs):
            seen.append(ip)
            return found(ip) if ip==NEW else None
        with patch("identity_discovery.socket.getaddrinfo",
                   return_value=answer(NEW)), \
             patch.object(devices,"ThreadPoolExecutor",ImmediatePool), \
             patch.object(devices,"probe",side_effect=fake_probe):
            self.book._refresh()
        saved=self.book.devices[self.cfg]
        self.assertEqual(seen,[OLD,NEW])
        self.assertEqual(saved["ip"],NEW)
        self.assertEqual(saved["previous_ip"],OLD)
        self.assertTrue(saved["online"])
        self.assertEqual(saved["key"],"secret-key-existing")

    def test_dns_first_skips_254_address_recovery_scan(self):
        checked=[]
        def fake_probe(ip,**kwargs):
            checked.append(ip)
            return found(ip) if ip==NEW else None
        with patch("identity_discovery.socket.getaddrinfo",
                   return_value=answer(NEW)), \
             patch.object(devices,"ThreadPoolExecutor",ImmediatePool), \
             patch.object(devices,"probe",side_effect=fake_probe):
            self.book._recover()
        self.assertEqual(checked,[NEW])
        self.assertEqual(self.book.devices[self.cfg]["ip"],NEW)
        self.assertFalse(self.book.scanning)

    def test_foreign_device_from_hostname_never_steals_existing_key(self):
        with patch("identity_discovery.socket.getaddrinfo",
                   return_value=answer(NEW)), \
             patch.object(devices,"ThreadPoolExecutor",ImmediatePool), \
             patch.object(devices,"probe",return_value=found(NEW,WRONG)):
            self.book._refresh()
        self.assertFalse(self.book.devices[self.cfg]["online"])
        self.assertEqual(self.book.devices[self.cfg]["ip"],OLD)
        self.assertEqual(self.book.devices[self.cfg]["key"],"secret-key-existing")

    def test_unpaired_announcement_is_discovered_not_auto_owned(self):
        # This is a second ESP32 not yet owned by the Builder.
        self.book.devices[self.cfg]["physical_id"]=WRONG
        discovered=found(NEW)
        self.assertTrue(self.book.accept_announced_device(discovered))
        saved=self.book.devices[ID]
        self.assertFalse(saved["managed"])
        self.assertNotIn("key",saved)
        self.assertTrue(saved["online"])
        self.assertEqual(saved["hostname"],"rflink-68ee8f479058")
        self.assertEqual(self.book.devices[self.cfg]["key"],"secret-key-existing")
        self.assertNotIn("secret-key-existing",
                         json.dumps(self.book.status()))

    def test_announcement_updates_known_device_ip_without_duplication(self):
        self.assertTrue(self.book.accept_announced_device(found(NEW)))
        saved=self.book.devices[self.cfg]
        self.assertEqual(saved["ip"],NEW)
        self.assertTrue(saved["online"])
        self.assertEqual(saved["key"],"secret-key-existing")
        self.assertEqual(len(self.book.devices),1)

    def test_two_live_claims_of_same_mac_block_relink(self):
        self.book.devices[self.cfg]["online"]=True
        with patch.object(devices,"probe",return_value=found(OLD)):
            self.assertFalse(self.book.accept_announced_device(found(NEW)))
        saved=self.book.devices[self.cfg]
        self.assertFalse(saved["online"])
        self.assertEqual(saved["ip"],OLD)
        self.assertEqual(saved["discovery_hint"],"duplicate_identity")

    def test_unpaired_duplicate_announcement_does_not_flip_between_ips(self):
        self.book.devices.pop(self.cfg)
        self.assertTrue(self.book.accept_announced_device(found(OLD)))
        with patch.object(devices,"probe",return_value=found(OLD)):
            self.assertFalse(self.book.accept_announced_device(found(NEW)))
        self.assertFalse(self.book.devices[ID]["online"])
        self.assertEqual(self.book.devices[ID]["ip"],OLD)
        self.assertEqual(self.book.devices[ID]["discovery_hint"],"duplicate_identity")

    def test_invalid_hint_is_rejected_without_network_io(self):
        with patch.object(devices,"probe") as maybe_probe:
            self.assertFalse(self.book.accept_announced_device(found(
                "8.8.8.8")))
            self.assertFalse(self.book.accept_announced_device(found(
                NEW,WRONG,board_hash="")) )
            maybe_probe.assert_not_called()


class FakeInfo:
    port=80
    properties={b"id":ID.encode()}
    def parsed_addresses(self,ip_version):
        return [NEW,"8.8.8.8"]


class FakeZeroconf:
    def get_service_info(self,service_type,name,timeout=1000):
        return FakeInfo()


class AnnounceListenerTests(unittest.TestCase):
    def test_passive_dns_sd_validates_http_before_book_receives_device(self):
        with tempfile.TemporaryDirectory() as dirname:
            book=DeviceBook(dirname)
            listener=annonces.RFLinkAnnouncements(book)
            listener.zeroconf=FakeZeroconf()
            listener.ip_version=type("IPv4Only",(),{"V4Only":object()})
            with patch.object(annonces,"probe",return_value=found()):
                listener._resolve_and_probe(
                    annonces.SERVICE,"rflink-68ee8f479058."+annonces.SERVICE)
            self.assertIn(ID,book.devices)
            self.assertFalse(book.devices[ID]["managed"])
            self.assertEqual(book.devices[ID]["ip"],NEW)

    def test_unrelated_mdns_http_services_are_ignored(self):
        with tempfile.TemporaryDirectory() as dirname:
            book=DeviceBook(dirname)
            listener=annonces.RFLinkAnnouncements(book)
            listener.pool=type("P",(),{"submit":lambda *args:None})()
            listener._schedule("_http._tcp.local.",
                               "printer._http._tcp.local.")
            self.assertEqual(listener.pending,set())
            listener.zeroconf=FakeZeroconf()
            listener.ip_version=type("IPv4Only",(),{"V4Only":object()})
            with patch.object(annonces,"probe") as probe:
                listener._resolve_and_probe(annonces.SERVICE,
                    "printer._rflink._tcp.local.")
                probe.assert_not_called()
            self.assertEqual(book.devices,{})


class BuildContractTests(unittest.TestCase):
    def test_native_service_is_passive_non_secret_no_radio_mode_changes(self):
        fw=(ROOT/"firmware/main/rflink_mdns.cpp").read_text()
        web=(ROOT/"firmware/main/rflink_web_ota.cpp").read_text()
        cmake=(ROOT/"firmware/main/CMakeLists.txt").read_text()
        manifest=(ROOT/"firmware/main/idf_component.yml").read_text()
        self.assertIn('mdns_service_add(nullptr,"_rflink","_tcp",80',fw)
        self.assertIn("rflink_set_dhcp_hostname(station)",web)
        self.assertIn("rflink_start_service_discovery()",web)
        self.assertIn("rflink_mdns.cpp",cmake)
        self.assertIn("espressif/mdns",manifest)
        self.assertNotIn("password",fw.lower())
        self.assertNotIn("ota_key",fw.lower())

    def test_builder_ui_and_host_listener_have_bounded_discovery(self):
        server=(ROOT/"rflink_builder/server.py").read_text()
        self.assertIn("elif device_book.network and not any(",server)
        self.assertIn("RFLinkAnnouncements(device_book).start()",server)
        app=(ROOT/"rflink_builder/static/app.js").read_text()
        docker=(ROOT/"rflink_builder/Dockerfile").read_text()
        self.assertIn("zeroconf==0.154.0",docker)
        self.assertIn("dns_domain:deviceDnsDomain.value.trim()",app)
        self.assertIn("MDNS_WORKERS=2",
                      (ROOT/"rflink_builder/discovery_announcements.py").read_text())
        self.assertIn("version: 0.6.50",
                      (ROOT/"rflink_builder/config.yaml").read_text())


if __name__=="__main__":
    unittest.main()
