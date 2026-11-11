"""Memory-safe RFLink discovery regression tests (no real LAN traffic)."""
from concurrent.futures import Future
import io
import json
from pathlib import Path
import sys
import tempfile
import threading
import unittest
from unittest.mock import patch

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'rflink_builder'))
import devices
from devices import DeviceBook

class ImmediatePool:
    """Test double: executes scan tasks immediately, records worker cap."""
    opened=[]
    def __init__(self,max_workers):
        self.max_workers=max_workers
        self.opened.append(max_workers)
    def __enter__(self):return self
    def __exit__(self,*args):return False
    def submit(self,fn,*args,**kwargs):
        future=Future()
        try:future.set_result(fn(*args,**kwargs))
        except Exception as exc:future.set_exception(exc)
        return future

def native_device(ip):
    return dict(id='rf-123456abcdef',ip=ip,name='RFLink '+ip,online=True,
                board_id='rflink-s3',board_hash='f00dcafe',firmware='0.7')

class DiscoveryMemoryTests(unittest.TestCase):
    def setUp(self):
        ImmediatePool.opened=[]

    def test_bounded_workers_full_scan_preserves_both_default_subnets(self):
        with tempfile.TemporaryDirectory() as root:
            book=DeviceBook(root)
            contacted=[]
            def fake_probe(ip,**kwargs):
                contacted.append(ip)
                return native_device(ip) if ip=='192.168.178.42' else None
            with patch.object(devices,'ThreadPoolExecutor',ImmediatePool),\
                 patch.object(devices,'probe',side_effect=fake_probe):
                book._scan()
            self.assertEqual(ImmediatePool.opened,[devices.DISCOVERY_WORKERS])
            self.assertLessEqual(devices.DISCOVERY_WORKERS,12)
            self.assertEqual(len(contacted),508)
            self.assertIn('192.168.178.42',contacted)
            self.assertIn('192.168.1.55',contacted)
            self.assertEqual(len(book.status()['devices']),1)
            self.assertEqual(book.status()['devices'][0]['id'],'rf-123456abcdef')
            self.assertFalse(book.scanning)

    def test_custom_vlan_network_and_managed_saved_ip_stay_discoverable(self):
        with tempfile.TemporaryDirectory() as root:
            book=DeviceBook(root)
            book.set_network('10.23.45.0/24')
            ip='192.168.178.210'
            book.devices['ip:'+ip]={'id':'ip:'+ip,'ip':ip,'name':'My RFLink',
                'managed':True,'online':True,'key':'sensitive-ota-pass'}
            contacted=[]
            def fake_probe(candidate,**kwargs):
                contacted.append(candidate)
                return None
            with patch.object(devices,'ThreadPoolExecutor',ImmediatePool),\
                 patch.object(devices,'probe',side_effect=fake_probe):
                book._scan()
            self.assertEqual(len(contacted),255)
            self.assertIn('10.23.45.1',contacted)
            self.assertIn('10.23.45.254',contacted)
            self.assertIn(ip,contacted)
            self.assertNotIn('192.168.1.1',contacted)
            self.assertEqual(book.devices['ip:'+ip]['key'],'sensitive-ota-pass')
            self.assertTrue(book.devices['ip:'+ip]['managed'])
            self.assertFalse(book.devices['ip:'+ip]['online'])

    def test_background_health_check_uses_smaller_worker_pool(self):
        with tempfile.TemporaryDirectory() as root:
            book=DeviceBook(root)
            ip='192.168.178.42'
            book.devices['rf-123456abcdef']={
                **native_device(ip),'managed':True,'key':'saved-ota-key',
                'physical_id':'rf-123456abcdef','name':'Living room'}
            with patch.object(devices,'ThreadPoolExecutor',ImmediatePool),\
                 patch.object(devices,'probe',return_value=native_device(ip)):
                book._refresh()
            self.assertEqual(ImmediatePool.opened,[devices.HEALTH_WORKERS])
            self.assertLessEqual(devices.HEALTH_WORKERS,4)
            self.assertTrue(book.devices['rf-123456abcdef']['online'])
            self.assertEqual(book.devices['rf-123456abcdef']['name'],'Living room')
            self.assertEqual(book.devices['rf-123456abcdef']['key'],'saved-ota-key')

    def test_each_worker_reuses_same_strict_http_opener(self):
        status=json.dumps(dict(board_id='rflink-test',board_hash='00000000',
                               firmware='0.7',device_id='rf-123456abcdef')).encode()
        class FakeOpener:
            def __init__(self):self.count=0
            def open(self,request,timeout=None):
                self.count+=1
                self.assert_timeout=timeout
                return io.BytesIO(status)
        opener=FakeOpener()
        with patch.object(devices,'_probe_thread',threading.local()),\
             patch.object(devices,'build_opener',return_value=opener) as constructor:
            one=devices.probe('192.168.178.15')
            two=devices.probe('192.168.178.16')
        self.assertEqual(constructor.call_count,1)
        self.assertEqual(opener.count,2)
        self.assertEqual(one['id'],two['id'])
        self.assertEqual(one['ip'],'192.168.178.15')
        self.assertEqual(two['ip'],'192.168.178.16')

    def test_idle_release_callback_after_scan_and_short_health_check(self):
        from unittest.mock import Mock
        with tempfile.TemporaryDirectory() as root:
            complete = Mock()
            book = DeviceBook(root, on_activity_finished=complete)
            with patch.object(devices, 'ThreadPoolExecutor', ImmediatePool), \
                 patch.object(devices, 'probe', return_value=None):
                book._refresh()
                book._scan()
            self.assertEqual(complete.call_count, 2)
            self.assertFalse(book.refreshing)
            self.assertFalse(book.scanning)

    def test_allocator_limit_is_configured_for_builder_process(self):
        docker=(ROOT/'rflink_builder/Dockerfile').read_text('utf-8')
        self.assertIn('MALLOC_ARENA_MAX=2',docker)
        self.assertIn('DISCOVERY_WORKERS = 12',(ROOT/'rflink_builder/devices.py').read_text('utf-8'))

if __name__=='__main__':unittest.main()
