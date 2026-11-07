"""Functional per-device GitHub Release selection and install-scope regressions.

All tests run offline; GitHub source resolution is stubbed but the real
REST handler, CSRF and DeviceBook persistence are exercised. No RF hardware.
"""
import importlib.util
import json
from http.client import HTTPConnection
from http.server import ThreadingHTTPServer
import os
from pathlib import Path
import sys
import tempfile
import threading
import unittest
from unittest.mock import patch

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'rflink_builder'))
from devices import DeviceBook

TAG_ONE='firmware/v0.7.0-exp.1'
TAG_TWO='firmware/v0.7.0-exp.2'
DEVICE='cfg-release-pinned'
OTHER='cfg-release-other'
SHA='a'*40

def verified(tag,n):
    return {'tag':tag,'release_id':n,'repo':'juulius/rflink-universal',
            'sha':SHA,'prerelease':True}

def fixture():
    return {'id':DEVICE,'name':'Test RFLink','managed':True,'configured':True,
            'provisioned':True,'physical_id':'rf-abc123abc123',
            'board_id':'rflink-s3','board_hash':'1a2b3c4d',
            'board':{'board_id':'rflink-s3'},'wifi_ssid':'private-wifi',
            'wifi_password':'do-not-show','key':'private-ota-password',
            'config_revision':'keep-me-unchanged',
            'firmware_choice':verified(TAG_ONE,101)}

class DeviceChoiceStoreTests(unittest.TestCase):
    def test_switch_versions_persists_and_preserves_hardware_and_credentials(self):
        with tempfile.TemporaryDirectory() as tmp:
            store=DeviceBook(Path(tmp))
            store.devices[DEVICE]=fixture()
            result=store.select_firmware(DEVICE,verified(TAG_TWO,102))
            self.assertEqual(result['firmware_choice']['tag'],TAG_TWO)
            persisted=DeviceBook(Path(tmp)).get_config(DEVICE)
            self.assertEqual(persisted['firmware_choice']['tag'],TAG_TWO)
            self.assertEqual(persisted['board_hash'],'1a2b3c4d')
            self.assertEqual(persisted['config_revision'],'keep-me-unchanged')
            self.assertEqual(persisted['wifi_password'],'do-not-show')
            self.assertEqual(persisted['key'],'private-ota-password')
            store.select_firmware(DEVICE,'current')
            self.assertEqual(store.get_config(DEVICE)['firmware_choice'],'current')

    def test_unconfigured_or_unverified_choice_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            store=DeviceBook(Path(tmp))
            store.devices[DEVICE]=fixture()
            store.devices[OTHER]={'id':OTHER,'managed':True,'configured':False}
            for bad in ({'tag':TAG_ONE}, {'tag':TAG_ONE,'sha':SHA,'repo':'r','release_id':True},
                        '',{'name':TAG_ONE}):
                with self.subTest(bad=bad),self.assertRaises(ValueError):
                    store.select_firmware(DEVICE,bad)
            with self.assertRaises(ValueError):
                store.select_firmware(OTHER,verified(TAG_TWO,102))

class DeviceChoiceHttpTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp=tempfile.TemporaryDirectory(prefix='rflink-release-choice-')
        os.environ['RFLINK_DATA']=cls.tmp.name
        os.environ['RFLINK_DEV_MODE']='1'
        spec=importlib.util.spec_from_file_location(
            'rflink_device_release_choice_server',ROOT/'rflink_builder/server.py')
        cls.srv=importlib.util.module_from_spec(spec)
        spec.loader.exec_module(cls.srv)
        class FakeBuilder:
            def __init__(self):
                self.lock=threading.RLock()
                self.status='idle'
        cls.srv.builder=FakeBuilder()
        cls.srv.device_book.devices={DEVICE:fixture(),OTHER:{
            'id':OTHER,'managed':True,'configured':True,'firmware_choice':'current',
            'board_id':'rflink-s3','board_hash':'1a2b3c4d','config_revision':'different'
        }}
        cls.http=ThreadingHTTPServer(('127.0.0.1',0),cls.srv.Handler)
        cls.http.daemon_threads=True
        cls.thread=threading.Thread(target=cls.http.serve_forever,daemon=True)
        cls.thread.start()

    @classmethod
    def tearDownClass(cls):
        cls.http.shutdown();cls.http.server_close()
        cls.thread.join(timeout=5)
        cls.tmp.cleanup()

    def post(self,device=DEVICE,tag=TAG_TWO,rel_id=102,csrf=True):
        conn=HTTPConnection('127.0.0.1',self.http.server_port,timeout=5)
        headers={'Content-Type':'application/json'}
        if csrf:headers['X-RFLink-CSRF']=self.srv.CSRF
        payload=json.dumps({'device_id':device,'firmware':{
            'tag':tag,'release_id':rel_id}})
        try:
            conn.request('POST','/api/devices/firmware/select',payload,headers)
            res=conn.getresponse()
            return res.status,json.loads(res.read())
        finally:conn.close()

    def test_http_release_selector_uses_verified_commit_and_saved_device_only(self):
        with patch.object(self.srv.release_history,'resolve',
            side_effect=lambda repo,tag,ident:verified(tag,ident)
        ) as resolve:
            code,data=self.post()
        self.assertEqual(code,200,data)
        resolve.assert_called_once_with('juulius/rflink-universal',TAG_TWO,102)
        self.assertEqual(data['firmware_choice']['tag'],TAG_TWO)
        self.assertEqual(self.srv.device_book.get_config(DEVICE)['config_revision'],
                         'keep-me-unchanged')
        self.assertEqual(self.post(csrf=False)[0],403)

    def test_refuses_change_during_active_compilation(self):
        builder=self.srv.builder
        with builder.lock:
            prior=builder.status
            builder.status='running'
            try:
                code,body=self.post()
            finally:
                builder.status=prior
        self.assertEqual(code,409,body)

    def test_release_catalog_comes_from_published_releases_not_fake_stable(self):
        html=(ROOT/'rflink_builder/static/index.html').read_text()
        js=(ROOT/'rflink_builder/static/firmware_catalog.js').read_text()
        flow=(ROOT/'rflink_builder/static/device_firmware.js').read_text()
        self.assertEqual(html.count('data-firmware-channel='),3)
        self.assertIn('data-firmware-channel="stable"',html)
        self.assertIn('data-firmware-channel="pre-release"',html)
        self.assertIn('data-firmware-channel="experimental"',html)
        self.assertIn('api/releases?scope=all&page=',js)
        self.assertIn("api/devices/firmware/select",js)
        self.assertIn('aria-checked',js)
        self.assertIn('b.release_tag||null',flow)
        self.assertIn('detail-firmware-selected-version',html)
        self.assertIn('detail-install-panel',html)
        before_install=html.index('id="detail-install-panel"')
        self.assertLess(html.index('id="detail-firmware-channels"'),before_install)
        self.assertLess(html.index('id="detail-build"'),before_install)

if __name__=='__main__':
    unittest.main()
