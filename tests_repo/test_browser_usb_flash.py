"""PC browser USB flash: validation, build isolation and HTTP authorization tests.

These host tests never touch an ESP32 or send a USB command. Web Serial itself
must be verified in Chrome/Edge with the actual N16R8 board.
"""
from http.client import HTTPConnection
from http.server import ThreadingHTTPServer
import hashlib
import importlib.util
import json
import os
import shutil
from pathlib import Path
import sys
import tempfile
import threading
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'rflink_builder'))
from browser_flash import verified_parts, browser_factory_image, EXPECTED


class FactoryImageTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix='rflink-browser-test-')
        self.folder = Path(self.tmp.name)
        files = {}
        for key, offset in EXPECTED:
            size = {'bootloader': 4096, 'partition_table': 3072,
                    'provisioning_nvs': 0x10000, 'ota_data_initial': 0x2000,
                    'app_ota': 4096}[key]
            data = (b'\xe9' if key in ('bootloader', 'app_ota') else b'P') + b'A'*(size-1)
            name = key+'.bin'
            (self.folder/name).write_bytes(data)
            files[key] = {'filename': name, 'sha256': hashlib.sha256(data).hexdigest(),
                          'bytes' if key=='provisioning_nvs' else 'size': size}
            if key=='provisioning_nvs':
                files[key]['offset'] = 0x9000
        self.manifest = {'test_only': True, 'chip': 'esp32s3',
                         'flash_size_bytes': 16*1024*1024,
                         'bootloader_offset': 0, 'partition_table_offset': 0x8000,
                         'nvs_offset': 0x9000, 'nvs_size_bytes': 0x10000,
                         'otadata_offset': 0x19000, 'ota_0_offset': 0x20000,
                         'ota_slot_size_bytes': 0x400000,
                         'builder_device_id': 'cfg-test',
                         'files': files}

    def tearDown(self):
        self.tmp.cleanup()

    def test_five_verified_parts_have_exact_first_install_offsets(self):
        self.assertEqual([o for o,_ in verified_parts(self.manifest,self.folder)],
                         [0, 0x8000, 0x9000, 0x19000, 0x20000])

    def test_wrong_chip_layout_provisioning_and_sha_are_rejected(self):
        for field, wrong in [('chip','esp32'),('test_only',False),
                             ('flash_size_bytes',4*1024*1024),
                             ('nvs_offset',0x10000),('builder_device_id','')]:
            with self.subTest(field=field):
                changed=dict(self.manifest, **{field:wrong})
                with self.assertRaises(ValueError):
                    verified_parts(changed,self.folder)
        broken=json.loads(json.dumps(self.manifest))
        broken['files']['app_ota']['sha256']='0'*64
        with self.assertRaisesRegex(ValueError,'SHA-256'):
            verified_parts(broken,self.folder)
        broken=json.loads(json.dumps(self.manifest))
        broken['files'].pop('provisioning_nvs')
        with self.assertRaisesRegex(ValueError,'Incomplete'):
            verified_parts(broken,self.folder)
        broken=json.loads(json.dumps(self.manifest))
        broken['files']['bootloader']['filename']='../secrets'
        with self.assertRaisesRegex(ValueError,'Unsafe'):
            verified_parts(broken,self.folder)

    def test_merge_uses_real_esptool_and_only_verified_images(self):
        def mocked_run(command, **kwargs):
            self.assertIn('merge_bin',command)
            self.assertIn('--chip',command)
            self.assertIn('esp32s3',command)
            self.assertIn('--flash_size',command)
            self.assertIn('16MB',command)
            self.assertNotIn('write-flash',command)
            out=Path(command[command.index('-o')+1])
            merged=bytearray(b'\xff'*(0x20000+4096))
            for offset, file in verified_parts(self.manifest,self.folder):
                contents=file.read_bytes()
                merged[offset:offset+len(contents)]=contents
            out.write_bytes(merged)
            class Result: returncode=0
            return Result()
        with patch('browser_flash.subprocess.run',side_effect=mocked_run):
            data=browser_factory_image(self.manifest,self.folder,env={'PATH':'/bin'})
        self.assertEqual(data[0],0xe9)
        self.assertEqual(data[0x9000:0x9001],b'P')
        self.assertEqual(data[0x20000],0xe9)


class BrowserFlashApiTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp=tempfile.TemporaryDirectory(prefix='rflink-browser-api-')
        os.environ['RFLINK_DATA']=cls.tmp.name
        os.environ['RFLINK_DEV_MODE']='1'
        spec=importlib.util.spec_from_file_location('browser_flash_api_server',
            ROOT/'rflink_builder/server.py')
        cls.server=importlib.util.module_from_spec(spec)
        spec.loader.exec_module(cls.server)
        cls.server.DATA.mkdir(parents=True,exist_ok=True)
        (cls.server.PROJECT/'boards').mkdir(parents=True,exist_ok=True)
        (cls.server.PROJECT/'tools').mkdir(parents=True,exist_ok=True)
        shutil.copy2(ROOT/'firmware/boards/rflink_s3_n16r8.json',
                     cls.server.PROJECT/'boards/rflink_s3_n16r8.json')
        shutil.copy2(ROOT/'firmware/tools/generate_board_header.py',
                     cls.server.PROJECT/'tools/generate_board_header.py')
        cls.job='a'*12
        cls.device='cfg-browser-one'
        cls.other='cfg-browser-other'
        cls.file=Path(cls.tmp.name)/'release_manifest.json'
        cls.profile=json.loads((ROOT/'firmware/boards/rflink_s3_n16r8.json').read_text('utf-8'))
        cls.hash=cls.server.local_profile._validate(cls.profile)
        cls.manifest={'test_only':True,'chip':'esp32s3',
                      'builder_device_id':cls.device,'device_config_revision':'rev-one',
                      'board_profile_hash':cls.hash,'board_id':cls.profile['board_id']}
        cls.file.write_text(json.dumps(cls.manifest))
        class FakeBuilder:
            def __init__(self, outer):
                self.lock=threading.RLock()
                self.status='success';self.device_id=outer.device
                self.job_id=outer.job;self.manifest=str(outer.file)
        cls.server.builder=FakeBuilder(cls)
        cls.server.device_book.devices={
            cls.device: {'id':cls.device,'configured':True,'managed':True,
                         'board':cls.profile,'board_hash':cls.hash,'board_id':cls.profile['board_id'],
                         'config_revision':'rev-one'},
            cls.other: {'id':cls.other,'configured':True,'managed':True,
                        'board':cls.profile,'board_hash':cls.hash,'board_id':cls.profile['board_id'],
                        'config_revision':'rev-two'}
        }
        cls.httpd=ThreadingHTTPServer(('127.0.0.1',0),cls.server.Handler)
        cls.httpd.daemon_threads=True
        cls.thread=threading.Thread(target=cls.httpd.serve_forever,daemon=True)
        cls.thread.start()

    @classmethod
    def tearDownClass(cls):
        cls.httpd.shutdown();cls.httpd.server_close()
        cls.thread.join(timeout=5)
        cls.tmp.cleanup()

    def send(self,device=None,job=None,confirm='TESTBOARD',csrf=True):
        payload=json.dumps({'device_id':device or self.device,
                            'job_id':job or self.job,'confirm':confirm})
        headers={'Content-Type':'application/json'}
        if csrf: headers['X-RFLink-CSRF']=self.server.CSRF
        conn=HTTPConnection('127.0.0.1',self.httpd.server_port,timeout=5)
        try:
            conn.request('POST','/api/browser-flash/image',payload,headers)
            response=conn.getresponse()
            return response.status,response.read(),dict(response.getheaders())
        finally:conn.close()

    def test_private_binary_only_after_validated_device_and_csrf(self):
        with patch.object(self.server.toolchain,'is_ready',return_value=True), patch.object(
            self.server.toolchain,'environment',return_value={'PATH':'/bin'}), patch.object(
            self.server,'browser_factory_image',return_value=b'\xe9'+b'B'*65536
        ) as build:
            status,body,headers=self.send()
            self.assertEqual(status,200)
            self.assertEqual(body[:1],b'\xe9')
            self.assertEqual(headers['X-RFLink-Factory-Image'],'ESP32-S3-PRIVATE-TESTBOARD')
            build.assert_called_once()
            self.assertEqual(self.send(device=self.other)[0],409)
            self.assertEqual(self.send(job='b'*12)[0],409)
            self.assertEqual(self.send(confirm='')[0],400)
            self.assertEqual(self.send(csrf=False)[0],403)

    def test_modified_board_or_config_is_rejected_before_reading_secrets(self):
        for key, value in [('config_revision','other'),('board_hash','deadbeef')]:
            original=self.server.device_book.devices[self.device][key]
            self.server.device_book.devices[self.device][key]=value
            try:
                status,body,_=self.send()
                self.assertEqual(status,409)
                self.assertNotIn(b'B'*256,body)
            finally:self.server.device_book.devices[self.device][key]=original

    def test_ui_still_offers_host_usb_and_download_fallback(self):
        html=(ROOT/'rflink_builder/static/index.html').read_text()
        js=(ROOT/'rflink_builder/static/browser_flash.js').read_text()
        flow=(ROOT/'rflink_builder/static/device_firmware.js').read_text()
        self.assertIn('browser_flash.js',html)
        self.assertIn('data-install-method="browser"',html)
        self.assertIn('data-install-method="usb"',html)
        self.assertIn('data-install-method="download"',html)
        self.assertIn("method==='browser'",flow)
        self.assertIn('esphome-web-flash:firmware',js)
        self.assertIn("event.origin!==origin",js)
        self.assertIn('erase:true',js)
        self.assertIn('TESTBOARD',js)
        self.assertNotIn('navigator.serial.requestPort(',flow)


if __name__=='__main__':
    unittest.main()
