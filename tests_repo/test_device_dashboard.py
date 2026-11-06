"""Offline tests for RFLink discovery, adoption and per-device credential safety."""
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'rflink_builder'))
from devices import DeviceBook, local_ip, scan_network, probe

def fixture(ip, identity=None):
    return {'id':identity or 'ip:'+ip,'ip':ip,'name':'RFLink '+ip,
            'firmware':'0.7.0-n16r8-s20','board_id':'rflink-s3-n16r8-reva',
            'board_hash':'12345678','online':True}


class DeviceDashboardTests(unittest.TestCase):
    def test_only_lan_24_scans(self):
        self.assertEqual(scan_network('192.168.178.0/24'),'192.168.178.0/24')
        for bad in ('192.168.0.0/16','192.168.2.1/24','8.8.8.0/24','127.0.0.0/24','169.254.5.0/24'):
            with self.subTest(bad=bad), self.assertRaises(ValueError):
                scan_network(bad)
        for bad in ('8.8.8.8','127.0.0.1','169.254.1.2','not-an-ip'):
            with self.assertRaises(ValueError): local_ip(bad)

    def test_legacy_devices_have_different_passwords_and_hidden_output(self):
        with tempfile.TemporaryDirectory() as folder:
            book=DeviceBook(folder)
            first=fixture('192.168.178.21')
            second=fixture('192.168.178.22')
            book._store_found(first);book._store_found(second)
            book.adopt(first['id'],'RFLink Living Room')
            book.adopt(second['id'],'RFLink Attic')
            book.save_key(first['id'],'livingpassword123')
            book.save_key(second['id'],'atticpassword1234')
            self.assertEqual(book.get_target(first['id']),('192.168.178.21','livingpassword123'))
            self.assertEqual(book.get_target(second['id']),('192.168.178.22','atticpassword1234'))
            self.assertNotIn('livingpassword123',repr(book.status()))
            self.assertNotIn('atticpassword1234',repr(book.status()))
            self.assertEqual(Path(folder,'local','devices.json').stat().st_mode&0o777,0o600)
            book._store_found(first)
            self.assertEqual(book.status()['devices'][1]['name'],'RFLink Living Room')
            book.remove(first['id'])
            self.assertNotIn('livingpassword123',Path(folder,'local','devices.json').read_text())
            self.assertTrue(book.status()['devices'][0]['has_key'])

    def test_adoption_and_rename_validation(self):
        with tempfile.TemporaryDirectory() as folder:
            book=DeviceBook(folder)
            device=fixture('192.168.178.32')
            book._store_found(device)
            with self.assertRaises(ValueError): book.save_key(device['id'],'a-long-password')
            with self.assertRaises(ValueError): book.adopt(device['id'],'')
            book.adopt(device['id'],'RFLink Office')
            book.rename(device['id'],'RFLink Garage')
            self.assertEqual(book.status()['devices'][0]['name'],'RFLink Garage')
            with self.assertRaises(ValueError): book.rename(device['id'],'x'*60)

    def test_board_change_invalidates_old_key(self):
        with tempfile.TemporaryDirectory() as folder:
            book=DeviceBook(folder)
            device=fixture('192.168.178.23')
            book._store_found(device)
            book.adopt(device['id'],'RFLink 1')
            book.save_key(device['id'],'abcdefghijklmno')
            changed={**device,'board_hash':'new-hash'}
            book._store_found(changed)
            self.assertFalse(book.status()['devices'][0]['has_key'])

    def test_offline_refresh_keeps_saved_card(self):
        with tempfile.TemporaryDirectory() as folder:
            book=DeviceBook(folder)
            device=fixture('192.168.178.25')
            book._store_found(device)
            book.adopt(device['id'],'RFLink 1')
            with patch('devices.probe',return_value=None):
                book._refresh()
            current=book.status()['devices'][0]
            self.assertFalse(current['online'])
            self.assertTrue(current['managed'])
            self.assertIn('last_seen',current)

    def test_frontend_dashboard_is_wired(self):
        html=(ROOT/'rflink_builder/static/index.html').read_text()
        js=(ROOT/'rflink_builder/static/devices.js').read_text()
        self.assertIn('data-page="devices"',html)
        self.assertIn('id="dashboard-add"',html)
        self.assertIn('id="discovered-cards"',html)
        self.assertIn('src="devices.js"',html)
        self.assertIn("deviceRequest('adopt'",js)
        self.assertIn("deviceRequest('remove'",js)
        self.assertIn("latestFirmwareRelease",js)


if __name__ == '__main__': unittest.main()
