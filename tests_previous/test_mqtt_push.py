"""Tests for explicit MQTT profile transfer (no real network connection)."""
import importlib.util
import os
import json
import tempfile
from pathlib import Path
import unittest
from unittest.mock import patch

ROOT=Path(__file__).resolve().parents[1]
TD=tempfile.TemporaryDirectory()
os.environ['RFLINK_DATA']=TD.name
os.environ['RFLINK_DEV_MODE']='1'
spec=importlib.util.spec_from_file_location('rf_builder_push',ROOT/'rflink_builder/server.py')
mod=importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)

class FakeResponse:
    def __init__(self, data):self.data=json.dumps(data).encode()
    def __enter__(self):return self
    def __exit__(self,*_):return False
    def read(self,_=None):return self.data
class FakeOpener:
    def __init__(self):self.requests=[]
    def open(self,request,timeout=None):
        self.requests.append(request)
        if request.full_url.endswith('/api/mqtt/status'):
            return FakeResponse({'configured':False,'tls_supported':False})
        if request.full_url.endswith('/api/mqtt/config'):
            return FakeResponse({'ok':True,'restart_required':True})
        raise AssertionError(request.full_url)

class MqttPushTests(unittest.TestCase):
    @classmethod
    def tearDownClass(cls):TD.cleanup()
    def setUp(self):
        self.settings={'host':'192.168.1.50','port':1883,'username':'rf','password':'s3cret','tls':False}
        self.params={'ip':'192.168.1.98','key':'verylongpassword','confirm':'CONFIGURE'}
        self.device={'board_id':'rflink-s3-n16r8-reva','firmware':'0.7.0-n16r8-s20-b9d3226e6','ota_busy':False}
    def test_successful_push_never_returns_password(self):
        opener=FakeOpener()
        with patch.object(mod.mqtt_settings,'resolved',return_value=self.settings),\
             patch.object(mod,'http_device_json',return_value=self.device),\
             patch.object(mod,'build_opener',return_value=opener):
            result=mod.push_mqtt_config(self.params)
        self.assertTrue(result['ok'])
        self.assertEqual(len(opener.requests),2)
        post=opener.requests[1]
        self.assertEqual(post.full_url,'http://192.168.1.98/api/mqtt/config')
        self.assertEqual(post.get_header('X-rflink-update-key'),'verylongpassword')
        self.assertEqual(json.loads(post.data)['password'],'s3cret')
        self.assertNotIn('s3cret',json.dumps(result))
        self.assertNotIn('verylongpassword',json.dumps(result))
    def test_requires_explicit_confirmation(self):
        with self.assertRaisesRegex(ValueError,'CONFIGURE'):
            mod.push_mqtt_config({**self.params,'confirm':''})
    def test_refuses_inaccessible_broker_name(self):
        with patch.object(mod.mqtt_settings,'resolved',return_value={**self.settings,'host':'core-mosquitto'}):
            with self.assertRaisesRegex(ValueError,'LAN IPv4'):
                mod.push_mqtt_config(self.params)
    def test_refuses_tls_not_implemented(self):
        with patch.object(mod.mqtt_settings,'resolved',return_value={**self.settings,'tls':True}):
            with self.assertRaisesRegex(ValueError,'TLS'):
                mod.push_mqtt_config(self.params)
    def test_refuses_old_or_wrong_firmware(self):
        for bad in ({**self.device,'firmware':'0.6.6-RadioGaga'},
                    {**self.device,'board_id':'unknown'}):
            with patch.object(mod.mqtt_settings,'resolved',return_value=self.settings),\
                 patch.object(mod,'http_device_json',return_value=bad),\
                 patch.object(mod,'build_opener') as opens:
                with self.assertRaisesRegex(RuntimeError,'compatible'):
                    mod.push_mqtt_config(self.params)
                opens.assert_not_called()
    def test_refuses_ota_busy(self):
        with patch.object(mod.mqtt_settings,'resolved',return_value=self.settings),\
             patch.object(mod,'http_device_json',return_value={**self.device,'ota_busy':True}):
            with self.assertRaisesRegex(RuntimeError,'OTA'):
                mod.push_mqtt_config(self.params)
if __name__=='__main__':unittest.main()
