"""RFLink connection diagnostics and safe discovery (no LAN or hardware needed)."""
from pathlib import Path
from urllib.error import HTTPError, URLError
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'rflink_builder'))
from devices import DeviceBook
import devices


class DiscoveryDiagnosticsTests(unittest.TestCase):
    def test_http_404_is_not_reported_as_wrong_subnet(self):
        with tempfile.TemporaryDirectory() as folder:
            book=DeviceBook(folder)
            err=HTTPError('http://192.168.52.67/api/status',404,'Not found',{},None)
            with patch.object(devices,'probe',side_effect=err):
                with self.assertRaisesRegex(ValueError,'HTTP 404.*api/status'):
                    book.add('192.168.52.67')

    def test_vlan_firewall_failure_is_actionable(self):
        with tempfile.TemporaryDirectory() as folder:
            book=DeviceBook(folder)
            with patch.object(devices,'probe',side_effect=URLError('timed out')):
                with self.assertRaisesRegex(ValueError,'TCP/80'):
                    book.add('192.168.52.67')

    def test_invalid_native_status_is_distinguished_from_timeout(self):
        with tempfile.TemporaryDirectory() as folder:
            book=DeviceBook(folder)
            with patch.object(devices,'probe',return_value=None):
                with self.assertRaisesRegex(ValueError,'firmware, board_id and board_hash'):
                    book.add('192.168.52.67')

    def test_online_but_unlinked_config_does_not_steal_credentials(self):
        with tempfile.TemporaryDirectory() as folder:
            book=DeviceBook(folder)
            config='cfg-new-board'
            book.devices[config]={
                'id':config,'managed':True,'configured':True,'provisioned':True,
                'ip':'192.168.52.67','board_id':'rflink-s3-n16r8-reva',
                'board_hash':'5e871b06','key':'private-not-for-other-boards',
                'name':'Test RFLink'}
            detected={'id':'rf-112233445566','online':True,
                'ip':'192.168.52.67','board_id':'rflink-s3-n16r8-reva',
                'board_hash':'5e871b06','firmware':'0.7.0-exp.3-n16r8-abcdef'}
            with patch.object(devices,'probe',return_value=detected):
                book._refresh()
            result=book.devices[config]
            self.assertFalse(result['online'])
            self.assertEqual(result['discovery_hint'],'link_required')
            self.assertNotIn('physical_id',result)
            self.assertEqual(result['key'],'private-not-for-other-boards')

    def test_changed_board_identity_is_not_assumed_safe(self):
        with tempfile.TemporaryDirectory() as folder:
            book=DeviceBook(folder)
            book.devices['cfg-one']={'id':'cfg-one','managed':True,
                'configured':True,'provisioned':True,'ip':'192.168.52.67',
                'board_id':'rflink-s3-n16r8-reva','board_hash':'5e871b06'}
            result={'id':'rf-abcdefabcdef','ip':'192.168.52.67','online':True,
                'board_id':'rflink-s3-n16r8-reva','board_hash':'NOT-THE-SAME',
                'firmware':'0.7'}
            with patch.object(devices,'probe',return_value=result):
                book._refresh()
            self.assertFalse(book.devices['cfg-one']['online'])
            self.assertEqual(book.devices['cfg-one']['discovery_hint'],'identity_mismatch')


class WifiSourceAndUiTests(unittest.TestCase):
    def test_native_logs_ip_disconnect_and_identity_without_wifi_password(self):
        fw=(ROOT/'firmware/main/rflink_web_ota.cpp').read_text('utf-8')
        self.assertIn('Wi-Fi STA started; connecting',fw)
        self.assertIn('Wi-Fi associated with access point',fw)
        self.assertIn('Wi-Fi READY: IP=',fw)
        self.assertIn('Wi-Fi disconnected: reason=',fw)
        self.assertIn('ESP_MAC_WIFI_STA',fw)
        self.assertIn('source_commit_sha', (ROOT/'rflink_builder/server.py').read_text('utf-8'))
        self.assertIn('device_id',fw)
        self.assertIn('IP2STR(&ip.ip)',fw)
        self.assertIn('static_cast<int32_t>(ip.ip.addr)',fw)
        self.assertIn('"/api/status"',fw)
        self.assertIn('Wi-Fi disconnect reason',fw)
        self.assertNotIn('ESP_LOGI(TAG,"Wi-Fi password',fw)

    def test_device_ui_displays_real_subnet_and_safe_manual_linking(self):
        html=(ROOT/'rflink_builder/static/index.html').read_text('utf-8')
        js=(ROOT/'rflink_builder/static/devices.js').read_text('utf-8')
        app=(ROOT/'rflink_builder/static/app.js').read_text('utf-8')
        self.assertIn('id="wizard-scan-scope"',html)
        self.assertIn('v0.6.24',html)
        self.assertIn("result.network",app)
        self.assertIn("deviceRequest('link'",js)
        self.assertIn('The RFLink answers at this IP',js)
        self.assertIn('scan '+ 'network',html.lower())

if __name__=='__main__':unittest.main()
