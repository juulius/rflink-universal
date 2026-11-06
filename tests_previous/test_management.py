"""Host tests for deployment restrictions and broker management (no real flashing)."""
import importlib.util
import io
import json
import os
from pathlib import Path
import socket
import tempfile
import threading
import unittest
from unittest.mock import patch

ROOT=Path(__file__).resolve().parents[1]
TD=tempfile.TemporaryDirectory()
os.environ['RFLINK_DATA']=TD.name
os.environ['RFLINK_DEV_MODE']='1'
spec=importlib.util.spec_from_file_location('rf_builder_management',ROOT/'rflink_builder/server.py')
mod=importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)


class ManagementTests(unittest.TestCase):
    @classmethod
    def tearDownClass(cls):
        TD.cleanup()

    def test_private_lan_only_for_ota(self):
        for good in ('10.0.0.1','172.16.0.42','172.31.255.254','192.168.1.50'):
            self.assertEqual(mod.local_device_ip(good),good)
        for bad in ('127.0.0.1','localhost','8.8.8.8','169.254.1.1','192.0.0.9','0.0.0.0','::1'):
            with self.subTest(bad=bad),self.assertRaises(ValueError):
                mod.local_device_ip(bad)

    def test_usb_paths_allowlist_only(self):
        for bad in ('/dev/sda','/dev/ttyACM0;rm -rf /','/proc/self/mem','/dev/../../etc/shadow'):
            with self.assertRaises(ValueError):
                mod.valid_serial_port(bad)
        self.assertEqual(mod.serial_ports(), [x for x in mod.serial_ports() if x.startswith('/dev/')])

    def test_mqtt_manual_save_hides_password_and_permissions(self):
        s=mod.MQTTSettings()
        data=s.save({'host':'192.168.1.10','port':1883,'username':'rfdev','password':'secret','tls':False})
        self.assertEqual(data['source'],'manual')
        self.assertNotIn('password',data)
        self.assertTrue(data['password_saved'])
        self.assertEqual(mod.MQTT_FILE.stat().st_mode & 0o777,0o600)
        s.save({'host':'192.168.1.10','port':1883,'username':'rfdev','password':'','tls':False})
        self.assertEqual(s.resolved()['password'],'secret')
        s.reset()
        self.assertFalse(mod.MQTT_FILE.exists())

    def test_mqtt_invalid_settings(self):
        s=mod.MQTTSettings()
        for invalid in ({'host':'https://example.com'}, {'host':'a','port':0},
                        {'host':'host','port':65536},{'host':'host','username':'x'*500}):
            with self.assertRaises(ValueError):
                s.save(invalid)

    def test_mqtt_protocol_connect_with_fake_broker(self):
        server=socket.socket();server.bind(('127.0.0.1',0));server.listen(1)
        port=server.getsockname()[1];observed=[]
        def serve():
            try:
                c,_=server.accept();c.settimeout(2)
                with c:
                    request=c.recv(1024);observed.append(request)
                    c.sendall(bytes([0x20,0x02,0x00,0x00]))
            finally: server.close()
        thread=threading.Thread(target=serve,daemon=True);thread.start()
        result=mod.mqtt_check({'host':'127.0.0.1','port':port,'username':'demo','password':'secret','tls':False})
        thread.join(timeout=2)
        self.assertIn('succeeded',result)
        self.assertTrue(observed and observed[0].startswith(b'\x10'))

    def test_deployment_requires_build(self):
        mod.builder=mod.Builder()
        with self.assertRaisesRegex(RuntimeError,'successful ESP-IDF build'):
            mod.deployment.start('usb',{'port':'/dev/ttyACM0','confirm':'TESTBOARD'})

    def test_ota_aborts_before_upload_if_board_mismatch(self):
        release=Path(TD.name)/'releasefake';release.mkdir(exist_ok=True)
        (release/'app.bin').write_bytes(b'\xe9'+b'\0'*300)
        manifest={'board_id':'n16r8','board_profile_hash':'aabb','files':{'app_ota':{'filename':'app.bin'}}}
        device={'id':'ip:192.168.1.25','ip':'192.168.1.25','name':'Test RFLink',
                'board_id':'n16r8','board_hash':'aabb','firmware':'v0.7.0',
                'managed':True,'online':True,'key':'supersecretkey'}
        mod.device_book.devices[device['id']]=device
        with patch.object(mod,'http_device_json',return_value={'board_id':'other','board_hash':'aabb'}):
            with self.assertRaisesRegex(RuntimeError,'board profile'):
                mod.deployment._ota({'device_id':device['id']},manifest,release)

if __name__=='__main__': unittest.main(verbosity=2)
