"""Builder v0.6.21: USB-flashed ESP32 is auto-bound by real efuse MAC.

Only a successful USB flash of a validated device build registers the expected
hardware ID. Network discovery must report exactly that identity and the
expected app version, not merely the same IP or GPIO hash. No hardware or LAN
is contacted during these tests.
"""
from pathlib import Path
from unittest.mock import patch
import tempfile
import sys
import unittest

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'rflink_builder'))
from devices import DeviceBook, esptool_hardware_id

BOARD={'board_id':'rflink-s3-n16r8-reva'}
HASH='5e871b06'
MAC='rf-24d7eb123456'
WRONG='rf-24d7ebabcdef'
VERSION='0.7.0-exp.3-n16r8-b5e871b06'
OLD_VERSION='0.7.0-exp.2-n16r8-b5e871b06'
FW_CHOICE={'tag':'firmware/v0.7.0-exp.3','sha':'f'*40,
           'repo':'juulius/rflink-universal','release_id':314}

class AutoLinkTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory()
        self.book=DeviceBook(self.tmp.name)
        self.config_id=self.book.create_config('RFLink Universal',BOARD.copy(),HASH,
            'Test LAN','my-password-1234','my-private-ota-key-123456789',FW_CHOICE)['device_id']
        self.before=self.book.get_config(self.config_id)
        self.physical={'id':MAC,'ip':'192.168.52.67','name':'Found RFLink',
                       'online':True,'identity_verified':True,
                       'board_id':BOARD['board_id'],'board_hash':HASH,
                       'firmware':VERSION,'wifi_connected':True}

    def tearDown(self):
        self.tmp.cleanup()

    def stage(self, hardware_id=MAC, version=VERSION):
        with patch.object(self.book,'watch_for_flashing_hardware'):
            return self.book.expect_flashing_hardware(self.config_id,hardware_id,version)

    def test_flash_records_mac_without_claiming_online(self):
        state=self.stage()
        config=self.book.get_config(self.config_id)
        self.assertTrue(config['provisioned'])
        self.assertFalse(config['online'])
        self.assertEqual(config['expected_physical_id'],MAC)
        self.assertEqual(config['expected_firmware_version'],VERSION)
        self.assertFalse(config.get('physical_id'))
        self.assertEqual(len(state['devices']),1)
        self.assertNotIn('my-private-ota-key',str(state))

    def test_matching_firmware_and_efuse_mac_auto_link_without_duplicate(self):
        self.stage()
        self.book._store_found(self.physical)
        updated=self.book.get_config(self.config_id)
        self.assertTrue(updated['online'])
        self.assertEqual(updated['physical_id'],MAC)
        self.assertEqual(updated['firmware'],VERSION)
        self.assertEqual(updated['ip'],'192.168.52.67')
        self.assertTrue(updated['identity_verified'])
        self.assertFalse(updated.get('expected_physical_id'))
        self.assertEqual(len(self.book.status()['devices']),1)
        for key in ('name','board','board_hash','key','wifi_password',
                    'wifi_ssid','firmware_choice','config_revision'):
            self.assertEqual(updated[key],self.before[key],key)
        # Runtime state and identity survive a Home Assistant Builder restart.
        after_restart=DeviceBook(self.tmp.name).get_config(self.config_id)
        self.assertEqual(after_restart['physical_id'],MAC)
        self.assertEqual(after_restart['key'],self.before['key'])

    def test_early_network_diagnosis_must_not_complete_flash_enrollment(self):
        self.stage()
        # The HTTP API is usable before the three radio drivers and the
        # pending OTA health check finish. Do not claim the flash is verified.
        self.book._store_found({**self.physical,
                                'startup_complete': False,
                                'radio_initialized': False})
        waiting = self.book.get_config(self.config_id)
        self.assertFalse(waiting['online'])
        self.assertEqual(waiting['expected_physical_id'], MAC)
        self.assertEqual(waiting['discovery_hint'], 'startup_in_progress')
        self.assertEqual(waiting['key'], self.before['key'])
        self.assertEqual(len(self.book.status()['devices']), 1)
        self.book._store_found({**self.physical,
                                'startup_complete': True,
                                'radio_initialized': True})
        linked = self.book.get_config(self.config_id)
        self.assertTrue(linked['online'])
        self.assertEqual(linked['physical_id'], MAC)
        self.assertEqual(len(self.book.status()['devices']), 1)

    def test_same_ip_and_board_hash_but_other_mac_is_not_our_rf_link(self):
        self.stage()
        self.book._store_found({**self.physical,'id':WRONG})
        waiting=self.book.get_config(self.config_id)
        self.assertFalse(waiting['online'])
        self.assertFalse(waiting.get('physical_id'))
        self.assertEqual(waiting['expected_physical_id'],MAC)
        self.assertEqual(waiting['key'],self.before['key'])

    def test_wrong_version_must_not_claim_flash_success(self):
        self.stage()
        self.book._store_found({**self.physical,'firmware':OLD_VERSION})
        waiting=self.book.get_config(self.config_id)
        self.assertFalse(waiting['online'])
        self.assertEqual(waiting['discovery_hint'],'expected_version_mismatch')
        self.assertEqual(waiting['observed_firmware'],OLD_VERSION)
        self.book._store_found(self.physical)
        self.assertTrue(self.book.get_config(self.config_id)['online'])
        self.assertEqual(len(self.book.status()['devices']),1)

    def test_wrong_board_profile_not_bound(self):
        self.stage()
        self.book._store_found({**self.physical,'board_hash':'ffffffff'})
        waiting=self.book.get_config(self.config_id)
        self.assertFalse(waiting['online'])
        self.assertEqual(waiting['discovery_hint'],'hardware_profile_mismatch')
        self.assertEqual(waiting['key'],self.before['key'])

    def test_not_auto_bound_to_legacy_ip_only_identity(self):
        self.stage()
        self.book._store_found({**self.physical,'id':'ip:192.168.52.67',
                                'identity_verified':False})
        self.assertFalse(self.book.get_config(self.config_id)['online'])

    def test_two_same_board_configs_do_not_share_the_same_esp32(self):
        second=self.book.create_config('Another RFLink',BOARD.copy(),HASH,
            'Test LAN','my-password-1234','another-private-ota-key-123456789')['device_id']
        self.stage()
        with patch.object(self.book,'watch_for_flashing_hardware'):
            with self.assertRaisesRegex(ValueError,'another RFLink'):
                self.book.expect_flashing_hardware(second,MAC,VERSION)

    def test_bound_device_reflash_waits_for_new_network_response(self):
        self.stage()
        self.book._store_found(self.physical)
        self.assertTrue(self.book.get_config(self.config_id)['online'])
        self.stage()
        waiting=self.book.get_config(self.config_id)
        self.assertFalse(waiting['online'])
        self.assertEqual(waiting['expected_physical_id'],MAC)
        self.book._store_found(self.physical)
        self.assertTrue(self.book.get_config(self.config_id)['online'])

    def test_durable_pending_enrollment_survives_builder_restart(self):
        self.stage()
        restarted=DeviceBook(self.tmp.name)
        self.assertEqual(restarted.get_config(self.config_id)['expected_physical_id'],MAC)
        restarted._store_found(self.physical)
        self.assertTrue(restarted.get_config(self.config_id)['online'])

    def test_legacy_ip_bound_config_upgrades_after_ota_without_new_card(self):
        self.book._store_found({**self.physical,'id':'ip:192.168.52.67',
                                'firmware':OLD_VERSION,'identity_verified':False})
        self.book.link(self.config_id,'ip:192.168.52.67')
        self.assertEqual(self.book.get_config(self.config_id)['physical_id'],
                         'ip:192.168.52.67')
        self.book._store_found(self.physical)
        self.assertEqual(len(self.book.status()['devices']),2)
        self.book.migrate_verified_ota_identity(self.config_id,MAC,VERSION)
        updated=self.book.get_config(self.config_id)
        self.assertEqual(updated['physical_id'],MAC)
        self.assertEqual(updated['key'],self.before['key'])
        self.assertEqual(updated['firmware'],VERSION)
        self.assertEqual(len(self.book.status()['devices']),1)
        self.book._store_found(self.physical)
        self.assertEqual(len(self.book.status()['devices']),1)

    def test_manual_link_of_pre_id_firmware_clears_pending_wait(self):
        self.stage()
        legacy={**self.physical,'id':'ip:192.168.52.67',
                'firmware':OLD_VERSION,'identity_verified':False}
        self.book._store_found(legacy)
        self.book.link(self.config_id,legacy['id'])
        saved=self.book.get_config(self.config_id)
        self.assertTrue(saved['online'])
        self.assertEqual(saved['physical_id'],'ip:192.168.52.67')
        self.assertFalse(saved.get('expected_physical_id'))
        self.assertFalse(saved.get('expected_firmware_version'))
        self.assertEqual(saved['key'],self.before['key'])

    def test_wrong_existing_hardware_cannot_be_assigned(self):
        self.stage()
        self.book._store_found(self.physical)
        with patch.object(self.book,'watch_for_flashing_hardware'):
            with self.assertRaisesRegex(ValueError,'different ESP32'):
                self.book.expect_flashing_hardware(self.config_id,WRONG,VERSION)

    def test_read_mac_output_parser_is_strict(self):
        for value in ('MAC: 24:d7:eb:12:34:56\n',
                      'Base MAC: 24:D7:EB:12:34:56\n'):
            self.assertEqual(esptool_hardware_id(value),MAC)
        for value in ('random 24:d7:eb:12:34:56','MAC: aa:bb','MAC: 24:d7:eb:12:34:56:78'):
            self.assertIsNone(esptool_hardware_id(value))

    def test_browser_and_host_usb_are_wired_to_same_enrollment(self):
        js=(ROOT/'rflink_builder/static/rf_flash.js').read_text('utf-8')
        server=(ROOT/'rflink_builder/server.py').read_text('utf-8')
        devices=(ROOT/'rflink_builder/static/devices.js').read_text('utf-8')
        self.assertIn('loader.chip.readMac(loader)',js)
        self.assertIn("api/devices/usb-flash/identify",js)
        self.assertIn("api/devices/usb-flash/identify",server)
        self.assertIn("esptool_hardware_id(result.stdout)",server)
        self.assertIn("device_book.expect_flashing_hardware(",server)
        self.assertIn("migrate_verified_ota_identity(",server)
        self.assertIn("x.expected_physical_id===d.id",devices)
        self.assertIn("Waiting for ESP32",devices)

if __name__=='__main__':
    unittest.main()
