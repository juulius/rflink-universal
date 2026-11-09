"""Offline OTA transfer regression: real HTTP byte progress, separate from boot verification."""
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from http.client import HTTPConnection
from pathlib import Path
from unittest.mock import patch
import importlib.util
import os
import tempfile
import threading
import unittest

ROOT=Path(__file__).resolve().parents[1]


class OtaReceiver(BaseHTTPRequestHandler):
    def do_POST(self):
        self.server.endpoint=self.path
        self.server.headers_seen=dict(self.headers)
        size=int(self.headers.get('Content-Length','0'))
        self.server.payload=self.rfile.read(size)
        self.server.received.set()
        self.send_response(self.server.reply_status)
        self.end_headers()
        self.wfile.write(b'Update written. Restarting...' if self.server.reply_status==200
                         else b'Invalid firmware')
    def log_message(self, *_):
        pass


class OtaTransferTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp=tempfile.TemporaryDirectory(prefix='rflink-ota-progress-')
        with patch.dict(os.environ, {'RFLINK_DATA':cls.tmp.name,'RFLINK_DEV_MODE':'1'}):
            spec=importlib.util.spec_from_file_location(
                'rflink_test_ota_transfer_server',ROOT/'rflink_builder/server.py')
            cls.builder=importlib.util.module_from_spec(spec)
            spec.loader.exec_module(cls.builder)
        cls.httpd=ThreadingHTTPServer(('127.0.0.1',0),OtaReceiver)
        cls.httpd.reply_status=200
        cls.httpd.received=threading.Event()
        cls.thread=threading.Thread(target=cls.httpd.serve_forever,daemon=True)
        cls.thread.start()

    @classmethod
    def tearDownClass(cls):
        cls.httpd.shutdown();cls.httpd.server_close()
        cls.thread.join(timeout=5);cls.tmp.cleanup()

    def connect(self, _ip, timeout=90):
        return HTTPConnection('127.0.0.1',self.httpd.server_port,timeout=timeout)

    def test_real_http_stream_reports_transmitted_bytes_and_requires_ack(self):
        payload=b'RFLink-test-image' * 8500  # Multiple 32 KiB writes and a final partial chunk
        self.httpd.received.clear()
        self.httpd.reply_status=200
        progress=[]
        stages=[]
        result=self.builder.send_ota_firmware(
            '192.168.52.67','private-test-key',payload,
            lambda sent,total:progress.append((sent,total)),
            lambda:stages.append('sent'), connection_factory=self.connect)
        self.assertIn('Update written',result)
        self.assertTrue(self.httpd.received.is_set())
        self.assertEqual(self.httpd.payload,payload)
        self.assertEqual(self.httpd.endpoint,'/api/ota')
        self.assertEqual(self.httpd.headers_seen['Content-Length'],str(len(payload)))
        self.assertEqual(self.httpd.headers_seen['Content-Type'],'application/octet-stream')
        self.assertEqual(self.httpd.headers_seen['X-RFLink-Update-Key'],'private-test-key')
        self.assertNotIn('Transfer-Encoding',self.httpd.headers_seen)
        self.assertGreater(len(progress),2)
        self.assertEqual(progress[-1],(len(payload),len(payload)))
        self.assertTrue(all(0<sent<=total==len(payload) for sent,total in progress))
        self.assertEqual([sent for sent,_ in progress],sorted(set(sent for sent,_ in progress)))
        self.assertEqual(stages,['sent'])

    def test_http_error_is_not_reported_as_success(self):
        self.httpd.reply_status=400
        with self.assertRaisesRegex(RuntimeError,'RFLink OTA HTTP 400'):
            self.builder.send_ota_firmware(
                '192.168.52.67','private-test-key',b'A'*1024,
                lambda *_:None,connection_factory=self.connect)
        self.httpd.reply_status=200

    def test_empty_image_rejected_without_connection(self):
        with self.assertRaisesRegex(ValueError,'empty'):
            self.builder.send_ota_firmware('192.168.52.67','key',b'',lambda *_:None,
                connection_factory=lambda *_args,**_kwargs: self.fail('No connection expected'))

    def test_ui_keeps_upload_progress_separate_from_verified_boot(self):
        server=(ROOT/'rflink_builder/server.py').read_text('utf-8')
        flash=(ROOT/'rflink_builder/static/rf_flash.js').read_text('utf-8')
        chooser=(ROOT/'rflink_builder/static/firmware_catalog.js').read_text('utf-8')
        panel=(ROOT/'rflink_builder/static/device_firmware.js').read_text('utf-8')
        self.assertIn("'upload_bytes':self.upload_bytes",server)
        self.assertIn("'upload_total':self.upload_total",server)
        self.assertIn("phase='awaiting_ack'",server)
        self.assertIn("phase==='uploading'",flash)
        self.assertIn('sent*100/total',flash)
        self.assertIn("state==='success' && ota && deployment.verified===true",flash)
        self.assertIn('FIRMWARE_VISIBLE_DEFAULT=5',chooser)
        self.assertIn('firmwareVisibleLimit+=5',chooser)
        self.assertIn("showDeviceDetail(d.id,'firmware')",panel)


if __name__=='__main__':
    unittest.main()
