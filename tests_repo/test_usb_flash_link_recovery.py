"""Previously USB-flashed RFLinks can be linked to a saved Builder config.

Browser Web Serial never marks provisioned until real RF hardware is detected.
The Builder must not create a second managed device or discard credentials.
These tests use only local simulated RFLink records, no network or hardware.
"""
import tempfile
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[1]
import sys
sys.path.insert(0,str(ROOT/'rflink_builder'))
from devices import DeviceBook

BOARD={'board_id':'rflink-s3-n16r8-reva'}
HASH='5e871b06'
PHYSICAL='rf-001122334455'
CHOICE={'tag':'firmware/v0.7.0-exp.2','sha':'a'*40,
        'release_id':123,'repo':'juulius/rflink-universal'}

class USBFlashLinkRecoveryTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory()
        self.book=DeviceBook(self.tmp.name)
        self.id=self.book.create_config(
            'RFLink Universal', BOARD, HASH, 'HomeWifi',
            'wifi-private-password', 'device-ota-private-password-1234',
            CHOICE)['device_id']
        self.detected={
            'id':PHYSICAL, 'ip':'192.168.52.67', 'online':True,
            'board_id':BOARD['board_id'], 'board_hash':HASH,
            'name':'RFLink 192.168.52.67',
            'firmware':'0.7.0-exp.2-n16r8-b5e871b06',
            'identity_verified':True,'wifi_connected':True,
        }
        self.book._store_found(self.detected)

    def tearDown(self):
        self.tmp.cleanup()

    def test_link_unconfirmed_browser_usb_flash_preserves_entire_configuration(self):
        prior=self.book.get_config(self.id)
        self.assertFalse(prior['provisioned'])
        self.assertIsNone(prior.get('physical_id'))
        self.assertEqual(len(self.book.status()['devices']),2)
        self.book.link(self.id,PHYSICAL)
        result=self.book.get_config(self.id)
        self.assertTrue(result['provisioned'])
        self.assertEqual(result['physical_id'],PHYSICAL)
        self.assertEqual(result['ip'],'192.168.52.67')
        self.assertEqual(result['firmware'],self.detected['firmware'])
        self.assertEqual(result['name'],'RFLink Universal')
        self.assertTrue(result['wifi_connected'])
        self.assertEqual(result['discovery_hint'],'')
        for key in ('key','wifi_password','wifi_ssid','config_revision',
                    'firmware_choice','board','board_id','board_hash'):
            self.assertEqual(result[key],prior[key],key)
        self.assertEqual(len(self.book.status()['devices']),1)
        self.book._store_found(self.detected)
        self.assertEqual(len(self.book.status()['devices']),1)
        again=DeviceBook(self.tmp.name).get_config(self.id)
        self.assertEqual(again['physical_id'],PHYSICAL)
        self.assertEqual(again['key'],prior['key'])

    def test_duplicate_binding_is_blocked(self):
        self.book.link(self.id,PHYSICAL)
        other={**self.detected,'id':'rf-aabbccddeeff','ip':'192.168.52.68'}
        self.book._store_found(other)
        with self.assertRaisesRegex(ValueError,'already linked'):
            self.book.link(self.id,other['id'])
        self.assertEqual(self.book.get_config(self.id)['physical_id'],PHYSICAL)

    def test_mismatched_pinout_cannot_be_linked_or_take_credentials(self):
        bad={**self.detected,'board_hash':'deadbeef'}
        self.book.devices[PHYSICAL]=bad
        secret=self.book.get_config(self.id)['key']
        with self.assertRaisesRegex(ValueError,'hardware profile'):
            self.book.link(self.id,PHYSICAL)
        self.assertFalse(self.book.get_config(self.id)['provisioned'])
        self.assertEqual(self.book.get_config(self.id)['key'],secret)

    def test_ui_exposes_link_when_not_provisioned_instead_of_offering_add(self):
        dashboard=(ROOT/'rflink_builder/static/devices.js').read_text('utf-8')
        advanced=(ROOT/'rflink_builder/static/wizard.js').read_text('utf-8')
        self.assertIn('x.configured&&!x.physical_id',dashboard)
        self.assertIn('x.board_id===d.board_id&&x.board_hash===d.board_hash',dashboard)
        self.assertIn('d.configured && !d.physical_id',advanced)
        self.assertIn('x.board_id===d.board_id&&x.board_hash===d.board_hash',advanced)
        self.assertNotIn('x.configured&&x.provisioned&&!x.physical_id',dashboard)
        self.assertIn('deviceRequest(\'link\'',dashboard)
        self.assertIn('deviceRequest(\'link\'',advanced)
        self.assertIn('Matching pinout does not prove',advanced)

if __name__=='__main__':
    unittest.main()
