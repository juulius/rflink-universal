"""Device-first build popup / inline installation safety regressions (no hardware flash)."""
from html.parser import HTMLParser
from http.client import HTTPConnection
from http.server import ThreadingHTTPServer
import importlib.util
import json
import os
from pathlib import Path
import tempfile
import threading
import unittest

ROOT=Path(__file__).resolve().parents[1]
STATIC=ROOT/'rflink_builder/static'

class Elements(HTMLParser):
    def __init__(self):
        super().__init__();self.ids=set();self.scripts=[]
    def handle_starttag(self,tag,attrs):
        data=dict(attrs)
        if 'id' in data:self.ids.add(data['id'])
        if tag=='script':self.scripts.append(data.get('src',''))

class UIFlowTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.html=(STATIC/'index.html').read_text('utf-8')
        cls.firmware=(STATIC/'device_firmware.js').read_text('utf-8')
        cls.devices=(STATIC/'devices.js').read_text('utf-8')
        cls.wizard=(STATIC/'wizard.js').read_text('utf-8')
        cls.parser=Elements();cls.parser.feed(cls.html)

    def test_modal_and_device_install_controls_exist(self):
        for ident in ('firmware-build-dialog','firmware-build-log','firmware-build-install',
            'firmware-build-stop','firmware-build-close','detail-build',
            'detail-build-log','detail-ota','detail-install-panel',
            'detail-install-ota','detail-install-usb','detail-install-download',
            'detail-install-port','detail-install-confirm','detail-install-start',
            'detail-download','detail-install-log'):
            self.assertIn(ident,self.parser.ids)
        self.assertIn('device_firmware.js',self.parser.scripts)

    def test_only_advanced_tools_route_to_old_build_page(self):
        self.assertNotIn("document.getElementById('detail-build').addEventListener",self.devices)
        self.assertNotIn("document.getElementById('detail-ota').addEventListener",self.devices)
        self.assertIn("beginFirmwareBuild(id)",self.wizard)
        self.assertNotIn("navigate('build')",self.wizard)
        self.assertIn("beginFirmwareBuild(d.id)",self.firmware)
        self.assertIn("openDeviceInstallation(d)",self.firmware)
        self.assertIn("document.getElementById('advanced-open-build')",self.devices)

    def test_build_and_install_stay_device_scoped(self):
        self.assertIn("deviceBuildMatches(d,b)",self.firmware)
        self.assertIn("b.device_id===d.id",self.firmware)
        self.assertIn("b.config_revision===d.config_revision",self.firmware)
        self.assertIn("b.board_profile_hash!==d.board_hash",self.firmware)
        self.assertIn("fwRequest('api/releases/use'",self.firmware)
        self.assertIn("api/deploy/",self.firmware)
        self.assertIn("api/build/device",self.firmware)
        self.assertIn("api/download/device/usb?",self.firmware)
        self.assertIn("TESTBOARD",self.firmware)
        self.assertIn("firmwareDialog.close()",self.firmware)
        self.assertIn("pollDeviceFirmware()",self.firmware)

    def test_no_confusing_fake_compile_percentage(self):
        self.assertNotIn('percentComplete',self.firmware)
        self.assertNotIn("100%",self.firmware)

class DownloadScopeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data=tempfile.TemporaryDirectory(prefix='rflink-fw-download-')
        os.environ['RFLINK_DATA']=cls.data.name
        os.environ['RFLINK_DEV_MODE']='1'
        spec=importlib.util.spec_from_file_location('rflink_fw_download_server',ROOT/'rflink_builder/server.py')
        cls.server=importlib.util.module_from_spec(spec)
        spec.loader.exec_module(cls.server)
        cls.httpd=ThreadingHTTPServer(('127.0.0.1',0),cls.server.Handler)
        cls.httpd.daemon_threads=True
        cls.thread=threading.Thread(target=cls.httpd.serve_forever,daemon=True)
        cls.thread.start()
        cls.job='f'*12
        cls.one='cfg-firmware-one'
        cls.two='cfg-firmware-two'
        cls.folder=Path(cls.data.name)
        cls.package=cls.folder/'test-firmware.zip'
        cls.package.write_bytes(b'PRIVATE_TEST_DEVICE_PACKAGE')
        cls.manifest=cls.folder/'release_manifest.json'
        cls.manifest.write_text(json.dumps({
            'board_id':'rflink-s3','board_profile_hash':'1234abcd',
            'builder_device_id':cls.one,'device_config_revision':'rev-a'}),encoding='utf-8')
        class FakeBuilder:
            def __init__(self,outer):
                self.lock=threading.RLock()
                self.status='success';self.job_id=outer.job;self.device_id=outer.one
                self.bundle=str(outer.package);self.manifest=str(outer.manifest)
        cls.server.builder=FakeBuilder(cls)
        cls.server.device_book.devices={
            cls.one:{'id':cls.one,'managed':True,'configured':True,
                     'board_id':'rflink-s3','board_hash':'1234abcd','config_revision':'rev-a'},
            cls.two:{'id':cls.two,'managed':True,'configured':True,
                     'board_id':'rflink-s3','board_hash':'1234abcd','config_revision':'rev-b'}
        }

    @classmethod
    def tearDownClass(cls):
        cls.httpd.shutdown();cls.httpd.server_close();cls.thread.join(timeout=5)
        cls.data.cleanup()

    def fetch(self,device,job=None):
        conn=HTTPConnection('127.0.0.1',self.httpd.server_port,timeout=5)
        try:
            conn.request('GET','/api/download/device/usb?device_id='+device+
                         '&job_id='+(job or self.job))
            resp=conn.getresponse()
            return resp.status, resp.read()
        finally:conn.close()

    def test_exact_device_and_job_can_download(self):
        status,body=self.fetch(self.one)
        self.assertEqual(status,200)
        self.assertEqual(body,b'PRIVATE_TEST_DEVICE_PACKAGE')

    def test_cannot_download_other_device_image(self):
        status,body=self.fetch(self.two)
        self.assertEqual(status,409)
        self.assertNotIn(b'PRIVATE_TEST_DEVICE_PACKAGE',body)

    def test_cannot_download_wrong_job(self):
        status,body=self.fetch(self.one,'e'*12)
        self.assertEqual(status,409)
        self.assertNotIn(b'PRIVATE_TEST_DEVICE_PACKAGE',body)

    def test_stale_revision_is_blocked(self):
        original=self.server.device_book.devices[self.one]['config_revision']
        try:
            self.server.device_book.devices[self.one]['config_revision']='rev-new'
            status,body=self.fetch(self.one)
            self.assertEqual(status,409)
        finally:
            self.server.device_book.devices[self.one]['config_revision']=original

    def test_mismatched_pinout_is_blocked(self):
        original=self.server.device_book.devices[self.one]['board_hash']
        try:
            self.server.device_book.devices[self.one]['board_hash']='deadbeef'
            status,body=self.fetch(self.one)
            self.assertEqual(status,409)
        finally:
            self.server.device_book.devices[self.one]['board_hash']=original

if __name__=='__main__':unittest.main()
