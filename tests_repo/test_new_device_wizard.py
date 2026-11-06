"""Regression tests for configuring and provisioning independent RFLink boards."""
import json
import tempfile
from pathlib import Path
import sys
import unittest
from unittest.mock import patch
from types import SimpleNamespace

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'rflink_builder'))
from devices import DeviceBook
from provisioning import create_nvs_image, NVS_OFFSET, NVS_SIZE

BOARD={"board_id":"rflink-s3-n16r8-reva","mcu":"ESP32-S3-WROOM-1-N16R8",
       "spi":{"sclk":12,"mosi":11,"miso":13,"clock_hz":200000},
       "radios":[{"id":"43342","frequency_hz":433420000,"modulation":"ook","cs":4,"gdo0":5,"gdo2":6},
                 {"id":"43392","frequency_hz":433920000,"modulation":"ook","cs":9,"gdo0":8,"gdo2":14},
                 {"id":"86830","frequency_hz":868300000,"modulation":"fsk2","cs":15,"gdo0":16,"gdo2":17}]}

class NewDeviceConfigTests(unittest.TestCase):
    def test_generated_key_is_random_32_character_bearer_secret(self):
        with tempfile.TemporaryDirectory() as d:
            store=DeviceBook(d)
            a=store.generate_key()['key'];b=store.generate_key()['key']
            self.assertNotEqual(a,b)
            self.assertGreaterEqual(len(a),30)
            self.assertLessEqual(len(a),64)

    def test_two_profiles_are_independent_and_passwords_not_in_status(self):
        with tempfile.TemporaryDirectory() as d:
            store=DeviceBook(d)
            one=store.create_config('Living Room',BOARD.copy(),'1234abcd','Network','wifi-secret-one','ota-secret-for-living-room')
            two=store.create_config('Garage',BOARD.copy(),'1234abcd','Network','wifi-secret-two','ota-secret-for-the-garage')
            self.assertNotEqual(one['device_id'],two['device_id'])
            view=store.status()
            self.assertEqual(len(view['devices']),2)
            self.assertNotIn('ota-secret',json.dumps(view))
            self.assertNotIn('wifi-secret',json.dumps(view))
            self.assertFalse(view['devices'][0]['online'])
            self.assertTrue(view['devices'][0]['configured'])
            self.assertEqual(store.get_config(one['device_id'])['wifi_password'],'wifi-secret-one')
            self.assertNotEqual(store.get_config(one['device_id'])['config_revision'],
                                store.get_config(two['device_id'])['config_revision'])
            self.assertEqual((Path(d)/'local/devices.json').stat().st_mode & 0o777,0o600)

    def test_build_revision_changes_when_pinout_wifi_or_key_changes(self):
        with tempfile.TemporaryDirectory() as d:
            store=DeviceBook(d)
            item=store.create_config('RFLink',BOARD.copy(),'1234abcd','Network','abcdefgh','ota-secret-abcdefghijkl')
            ident=item['device_id']
            previous=store.get_config(ident)['config_revision']
            board=json.loads(json.dumps(BOARD))
            board['spi']['clock_hz']=300000
            store.update_config(ident,board,'aaaabbbb','Network','')
            changed=store.get_config(ident)
            self.assertNotEqual(previous,changed['config_revision'])
            previous=changed['config_revision']
            store.save_key(ident,'new-ota-secret-abcdefgh')
            self.assertNotEqual(previous,store.get_config(ident)['config_revision'])
            store.mark_provisioned(ident)
            with self.assertRaises(ValueError):
                store.save_key(ident,'another-ota-secret-abcdefg')
            with self.assertRaises(ValueError):
                store.update_config(ident,BOARD,'1234abcd','Network','')

    def test_link_requires_explicit_matching_physical_board(self):
        with tempfile.TemporaryDirectory() as d:
            store=DeviceBook(d)
            item=store.create_config('RFLink',BOARD.copy(),'1234abcd','Network','abcdefgh','ota-secret-abcdefghijkl')
            ident=item['device_id']
            physical={'id':'ip:192.168.178.50','ip':'192.168.178.50','name':'RFLink discovered',
                'firmware':'0.7.0','board_id':BOARD['board_id'],'board_hash':'1234abcd','online':True}
            store._store_found(physical)
            store.link(ident,physical['id'])
            profile=store.get_config(ident)
            self.assertEqual(profile['physical_id'],physical['id'])
            self.assertTrue(profile['provisioned'])
            self.assertEqual(len(store.status()['devices']),1)
            store._store_found(physical)
            self.assertEqual(len(store.status()['devices']),1)
            self.assertEqual(store.get_config(ident)['name'],'RFLink')

    def test_offline_devices_without_ip_keep_config(self):
        with tempfile.TemporaryDirectory() as d:
            store=DeviceBook(d)
            item=store.create_config('RFLink',BOARD.copy(),'1234abcd','Network','abcdefgh','ota-secret-abcdefghijkl')
            store._refresh()
            self.assertFalse(store.get_config(item['device_id'])['online'])

    def test_generate_nvs_uses_correct_namespace_and_offsets(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d)
            tool=root/'components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py'
            tool.parent.mkdir(parents=True)
            tool.write_text('# test stub')
            profile={'wifi_ssid':'Test WiFi','wifi_password':'abcdefgh',
                     'key':'unique-ota-password-12345'}
            captured=[]
            def fake_run(args,**kwargs):
                captured.extend(args)
                csv_data=Path(args[-3]).read_text()
                self.assertIn('rf_net,namespace,,',csv_data)
                self.assertIn('ssid,data,string,Test WiFi',csv_data)
                self.assertIn('pass,data,string,abcdefgh',csv_data)
                self.assertIn('update_key,data,string,unique-ota-password-12345',csv_data)
                Path(args[-2]).write_bytes(b'X'*NVS_SIZE)
                return SimpleNamespace(returncode=0)
            out=root/'release/provisioning.bin'
            with patch('provisioning.subprocess.run',side_effect=fake_run):
                result=create_nvs_image(profile,out,root)
            self.assertEqual(result['offset'],NVS_OFFSET)
            self.assertEqual(result['bytes'],NVS_SIZE)
            self.assertEqual(out.stat().st_size,NVS_SIZE)
            self.assertEqual(out.stat().st_mode & 0o777,0o600)
            self.assertNotIn('abcdefgh',str(captured))

    def test_frontend_routes_and_password_visibility_controls(self):
        html=(ROOT/'rflink_builder/static/index.html').read_text('utf-8')
        js=(ROOT/'rflink_builder/static/wizard.js').read_text('utf-8')
        self.assertIn('wizard-firmware',html)
        self.assertIn('wizard-generate-key',html)
        self.assertIn('wizard-inherit-wifi',html)
        self.assertIn('wizard-pin-grid',html)
        self.assertIn('wizard-finish-build',html)
        self.assertIn('beginFirmwareBuild(id)',js)
        self.assertIn('api/build/device',(ROOT/'rflink_builder/static/device_firmware.js').read_text('utf-8'))
        self.assertIn('api/devices/create',js)
        self.assertIn('api/devices/defaults',js)

if __name__=='__main__':
    unittest.main()
