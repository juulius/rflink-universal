#!/usr/bin/env python3
"""RFLink Universal Home Assistant ingress builder.

Only the authenticated HA ingress proxy can access the production service.
Users can submit source zip archives and invoke a fixed build pipeline.
No arbitrary commands, output paths, or shell interpolation are accepted.
"""
from __future__ import annotations

import hashlib
import http.client
import ipaddress
import io
import json
import re
import socket
import ssl
import struct
import os
import secrets
import sys
import shutil
import signal
import subprocess
import tempfile
import threading
import time
import uuid
import zipfile
from datetime import datetime, timezone
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path, PurePosixPath
from urllib.parse import urlparse, parse_qs
from urllib.request import Request, build_opener, ProxyHandler, HTTPRedirectHandler
from urllib.error import URLError, HTTPError
sys.path.insert(0,str(Path(__file__).resolve().parent))
from repo_sync import SourceStore, valid_repo, github_head, github_firmware_zip, github_release
from local_profile import LocalProfile
from local_secrets import LocalSecrets
from devices import DeviceBook, probe, local_ip, esptool_hardware_id
from release_history import ReleaseHistory, version_tuple
from toolchain import ToolchainManager
from memory_diagnostics import MemoryMonitor
from build_cleanup import discard_build_workspace
from provisioning import create_nvs_image, NVS_OFFSET, NVS_SIZE
from browser_flash import browser_factory_image
from diagnostic_proxy import diagnostics_for_device, crash_report_for_device, console_for_device
from firmware_identity import build_identity, require_selected_build
from ota_verification import OTAUnverified, OTARollback, confirm_ota_restart
from http_body import BodyReadError, read_body

APP = Path(__file__).resolve().parent
DATA = Path(os.environ.get('RFLINK_DATA', '/data')).resolve()
PROJECT = DATA / 'project'
RELEASES = DATA / 'releases'
DEFAULT_SOURCE = APP / 'default_source.zip'
PORT = int(os.environ.get('RFLINK_PORT', '8099'))
DEV_MODE = os.environ.get('RFLINK_DEV_MODE') == '1'
PROXY_IP = '172.30.32.2'  # Home Assistant Supervisor ingress proxy
MAX_UPLOAD = 32 * 1024 * 1024
MAX_EXPANDED = 96 * 1024 * 1024
MAX_ENTRIES = 2500
MAX_ENTRY = 12 * 1024 * 1024
MAX_BUILD_SECONDS = 55 * 60
MAX_LOG_CHUNK = 48 * 1024
CSRF = secrets.token_urlsafe(24)
MAX_JSON = 8192
GITHUB_CONFIG = DATA/'github_source.json'
DEFAULT_GITHUB_REPO = 'juulius/rflink-universal'
github_preview_revision = ''
MQTT_FILE = DATA / "mqtt_settings.json"
SERIAL_ALLOWED = re.compile(r"^/dev/(?:tty(?:USB|ACM)[0-9]{1,3}|serial/by-id/[a-zA-Z0-9_.:+-]+)$")


def utc_now() -> str:
    return datetime.now(timezone.utc).isoformat(timespec='seconds')


def inspect_zip(data: bytes) -> tuple[zipfile.ZipFile, list[tuple[zipfile.ZipInfo, str]]]:
    """Validate all members before extraction; return flattened root paths.

    Never call extractall() on untrusted source archives.
    """
    zf = zipfile.ZipFile(io.BytesIO(data))
    files = [m for m in zf.infolist() if not m.is_dir()]
    if not files or len(zf.infolist()) > MAX_ENTRIES:
        zf.close()
        raise ValueError('ZIP is empty or contains too many files')
    paths: list[tuple[zipfile.ZipInfo, str]] = []
    total = 0
    for info in files:
        raw = info.filename.replace('\\', '/')
        path = PurePosixPath(raw)
        if raw.startswith(('/', '//')) or '\x00' in raw or any(part in ('.', '..', '') for part in raw.split('/')):
            zf.close()
            raise ValueError('Invalid ZIP path: ' + raw[:80])
        if len(raw) > 240 or ':' in raw:
            zf.close()
            raise ValueError('ZIP path is not allowed')
        if ((info.external_attr >> 16) & 0o170000) == 0o120000:
            zf.close()
            raise ValueError('Symbolic links are not allowed in uploads')
        if info.file_size > MAX_ENTRY:
            zf.close()
            raise ValueError('File in ZIP is too large')
        total += info.file_size
        if total > MAX_EXPANDED:
            zf.close()
            raise ValueError('Expanded ZIP is too large')
        paths.append((info, path.as_posix()))
    # Projects can be in root or inside exactly one top-level enclosing directory.
    names = [path for _, path in paths]
    roots = {n.split('/')[0] for n in names}
    strip = len(roots) == 1 and all('/' in n for n in names)
    adjusted: list[tuple[zipfile.ZipInfo, str]] = []
    seen: set[str] = set()
    for info, path in paths:
        output = path.split('/', 1)[1] if strip else path
        if not output or output in seen:
            zf.close()
            raise ValueError('Dubbel of leeg pad in ZIP')
        seen.add(output)
        adjusted.append((info, output))
    required = {'CMakeLists.txt', 'main/CMakeLists.txt', 'sdkconfig.defaults',
                'partitions.csv', 'boards/rflink_s3_n16r8.json',
                'tools/preflight.py', 'tools/package_release.py'}
    missing = required - seen
    if missing:
        zf.close()
        raise ValueError('Incomplete RFLink ESP-IDF project: ' + ', '.join(sorted(missing)))
    return zf, adjusted


memory_monitor = MemoryMonitor()
memory_monitor.checkpoint("Python imports complete")
toolchain = ToolchainManager(DATA)
memory_monitor.checkpoint("Toolchain manager loaded")
release_history = ReleaseHistory()
source_store = SourceStore(DATA, PROJECT, inspect_zip)
local_profile = LocalProfile(DATA, PROJECT)
local_secrets = LocalSecrets(DATA)
device_book = DeviceBook(DATA)
memory_monitor.checkpoint("Saved settings and devices loaded")

def remove_device_and_private_builds(device_id):
    """Remove a saved device plus compiled NVS packages containing its keys."""
    if not isinstance(device_id,str) or not device_id:
        raise ValueError("Select a device")
    with builder.lock:
        if builder.device_id == device_id and builder.status in ("running","cancelling"):
            raise RuntimeError("Wait for the device build to finish before removing it")
        if builder.device_id == device_id and deployment.snapshot().get("status") == "running":
            raise RuntimeError("Wait for the device install to finish before removing it")
        for info in RELEASES.glob("*/build_info.json"):
            if not re.fullmatch(r"[a-f0-9]{12}",info.parent.name):
                continue
            try:
                meta=json.loads(info.read_text("utf-8"))
            except (OSError,ValueError):
                continue
            if isinstance(meta,dict) and meta.get("device_id")==device_id:
                shutil.rmtree(info.parent)
        if builder.device_id==device_id:
            builder.status="idle"
            builder.bundle=builder.ota=builder.manifest=""
            builder.device_id=None
        return device_book.remove(device_id)

def github_configuration():
    try:
        saved = json.loads(GITHUB_CONFIG.read_text('utf-8'))
        if isinstance(saved, dict) and saved.get('repo'):
            return {'repo': valid_repo(saved['repo'])}
    except (OSError, ValueError, TypeError):
        pass
    return {'repo': DEFAULT_GITHUB_REPO}

def save_github_repo(value):
    repo=valid_repo(value)
    DATA.mkdir(parents=True,exist_ok=True)
    GITHUB_CONFIG.write_text(json.dumps({'repo':repo},indent=2)+'\n',encoding='utf-8')
    return {'repo':repo}

def github_status():
    repo=github_configuration().get('repo','')
    if not repo:return {'configured':False,'repo':'','note':'Vul eigenaar/repository in'}
    sha=github_head(repo)
    latest=release_history.recent(repo).get('latest')
    published={'available':True,'version':latest['tag'],'published_at':latest['published_at']} if latest else {'available':False}
    return {'configured':True,'repo':repo,'revision':sha,'release':published}


def compare_github():
    global github_preview_revision
    repo=github_configuration()['repo']
    sha=github_head(repo)
    content=github_firmware_zip(repo,sha)
    result=source_store.preview(content,'GitHub '+repo+' @ '+sha[:12])
    github_preview_revision=sha
    result['revision']=sha
    return result

def compare_uploaded(data):
    return source_store.preview(data,'Uploaded ZIP')

def apply_comparison(token):
    result=source_store.apply(token)
    if builder:
        with builder.lock:
            builder.status='idle'; builder.bundle=builder.ota=builder.manifest=''
            builder.source={'name':'Updated RFLink source','uploaded_at':utc_now(),'sha256':token}
    return result

def install_source(data: bytes, name: str = 'RFLink source ZIP') -> dict:
    if not data or len(data) > MAX_UPLOAD:
        raise ValueError('ZIP groter dan 32 MB of leeg')
    zf, entries = inspect_zip(data)
    DATA.mkdir(parents=True, exist_ok=True)
    temporary = Path(tempfile.mkdtemp(prefix='rflink-upload-', dir=DATA))
    try:
        for info, rel in entries:
            dest = temporary.joinpath(*PurePosixPath(rel).parts)
            dest.parent.mkdir(parents=True, exist_ok=True)
            with zf.open(info) as src, dest.open('wb') as target:
                shutil.copyfileobj(src, target, 64 * 1024)
        # Only replace the project after full validation and extraction.
        previous = DATA / 'project_previous'
        if previous.exists():
            shutil.rmtree(previous)
        if PROJECT.exists():
            PROJECT.rename(previous)
        try:
            temporary.rename(PROJECT)
        except Exception:
            if previous.exists() and not PROJECT.exists():
                previous.rename(PROJECT)
            raise
        if previous.exists():
            shutil.rmtree(previous)
    finally:
        zf.close()
        if temporary.exists():
            shutil.rmtree(temporary)
    source_store.reset_baseline()
    return {'name': name[:100], 'sha256': hashlib.sha256(data).hexdigest(),
            'files': len(entries), 'size': len(data), 'uploaded_at': utc_now()}


class NoRedirect(HTTPRedirectHandler):
    """Never follow a redirect from a user-selected firmware destination."""
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        return None


def local_device_ip(value: str) -> str:
    """OTA only to an explicitly supplied private IPv4 address, no DNS/redirects."""
    try:
        addr = ipaddress.IPv4Address(value)
    except ipaddress.AddressValueError as exc:
        raise ValueError('Use the RFLink local IPv4 address, for example 192.168.1.50') from exc
    lan = (ipaddress.IPv4Network('10.0.0.0/8'),ipaddress.IPv4Network('172.16.0.0/12'),ipaddress.IPv4Network('192.168.0.0/16'))
    if not any(addr in block for block in lan):
        raise ValueError('Only a local private IPv4 address is allowed')
    return str(addr)


def http_device_json(ip: str) -> dict:
    opener = build_opener(ProxyHandler({}), NoRedirect())
    req = Request(f'http://{ip}/api/status', headers={'Accept': 'application/json'})
    with opener.open(req, timeout=5) as rsp:
        data = rsp.read(32769)
        if len(data) > 32768:
            raise RuntimeError('Apparaatstatus onverwacht groot')
        obj = json.loads(data)
    if not isinstance(obj, dict):
        raise ValueError('Invalid RFLink status response')
    return obj


def send_ota_firmware(ip, key, binary, on_progress, on_sent=lambda: None,
                      *, connection_factory=http.client.HTTPConnection):
    """Send exact Content-Length firmware bytes; report socket writes, not flash success.

    ESP-IDF's /api/ota handler needs a known content length. Streaming via
    http.client preserves this, avoids chunked transfer and does not follow
    redirects or use environment proxies. Only the final ESP32 HTTP reply
    confirms esp_ota_end() and esp_ota_set_boot_partition() succeeded.
    """
    total = len(binary)
    if not total:
        raise ValueError('OTA image is empty')
    conn = connection_factory(ip, timeout=90)
    try:
        def chunks():
            for offset in range(0, total, 32768):
                chunk = memoryview(binary)[offset:offset+32768]
                yield chunk
                # Execution resumes here after HTTPConnection.send returned.
                on_progress(offset+len(chunk), total)
        conn.request('POST', '/api/ota', body=chunks(), headers={
            'Content-Type': 'application/octet-stream',
            'Content-Length': str(total),
            'X-RFLink-Update-Key': key,
        })
        on_sent()  # All bytes passed to the socket; ESP32 flash acknowledgement pending.
        response = conn.getresponse()
        message = response.read(1024).decode('utf-8', 'replace')
        if response.status != 200:
            raise RuntimeError('RFLink OTA HTTP '+str(response.status)+': '+
                               message.replace(chr(13),' ').replace(chr(10),' ')[:160])
        return message
    finally:
        conn.close()


def serial_ports():
    ports = set()
    base = Path('/dev/serial/by-id')
    if base.is_dir():
        for item in base.iterdir():
            if SERIAL_ALLOWED.fullmatch(str(item)) and item.exists():
                ports.add(str(item))
    for prefix in ('ttyUSB','ttyACM'):
        for item in Path('/dev').glob(prefix + '*'):
            if SERIAL_ALLOWED.fullmatch(str(item)) and item.is_char_device():
                ports.add(str(item))
    return sorted(ports)


def valid_serial_port(value):
    if not isinstance(value,str) or not SERIAL_ALLOWED.fullmatch(value):
        raise ValueError('Ongeldige seriële poort')
    if value not in serial_ports():
        raise ValueError('Serial port is not available on this Home Assistant host')
    return value


def ha_broker_service():
    token = os.environ.get('SUPERVISOR_TOKEN') or os.environ.get('HASSIO_TOKEN')
    if not token:
        raise RuntimeError('Home Assistant Supervisor service is unavailable')
    req = Request('http://supervisor/services/mqtt', headers={'Authorization': f'Bearer {token}'})
    opener = build_opener(ProxyHandler({}))
    with opener.open(req, timeout=5) as rsp:
        obj = json.loads(rsp.read(8192))
    cfg = obj.get('data', obj)
    if not isinstance(cfg,dict) or not cfg.get('host'):
        raise ValueError('No MQTT broker found in Home Assistant')
    return {'host':str(cfg['host']), 'port':int(cfg.get('port',1883)),
            'username':str(cfg.get('username','')), 'password':str(cfg.get('password','')),
            'tls':bool(cfg.get('ssl', False))}


def mqtt_packet_string(data: str) -> bytes:
    raw = data.encode('utf-8')
    if len(raw) > 65535:
        raise ValueError('MQTT-veld te lang')
    return len(raw).to_bytes(2,'big')+raw


def mqtt_check(settings: dict) -> str:
    """Small MQTT 3.1.1 CONNECT/CONNACK test, no third-party dependency."""
    host, port = settings['host'], settings['port']
    username, password = settings.get('username',''),settings.get('password','')
    flags = 0x02 | (0x80 if username else 0) | (0x40 if password else 0)
    if password and not username:
        raise ValueError('MQTT password requires a username')
    body = mqtt_packet_string('MQTT') + bytes([4, flags, 0, 10]) + mqtt_packet_string('rflink-builder-test-' + secrets.token_hex(4))
    if username: body += mqtt_packet_string(username)
    if password: body += mqtt_packet_string(password)
    remaining, v = bytearray(), len(body)
    while True:
        n=v%128; v//=128
        remaining.append(n | (0x80 if v else 0))
        if not v: break
    packet = bytes([0x10])+bytes(remaining)+body
    with socket.create_connection((host,port),timeout=5) as raw:
        raw.settimeout(5)
        if settings.get('tls'):
            sock=ssl.create_default_context().wrap_socket(raw,server_hostname=host)
        else:
            sock=raw
        with sock:
            sock.sendall(packet)
            ack=b''
            while len(ack)<4:
                chunk=sock.recv(4-len(ack))
                if not chunk: break
                ack+=chunk
            if len(ack)!=4 or ack[:2]!=b'\x20\x02':
                raise RuntimeError('Broker did not send a valid MQTT CONNACK')
            if ack[3]!=0:
                raise RuntimeError(f'MQTT connection refused (code {ack[3]})')
            try: sock.sendall(b'\xe0\x00')
            except OSError: pass
    return 'Broker connection and authentication succeeded (from the Builder, not from RFLink)'


class MQTTSettings:
    def __init__(self):
        self.lock=threading.RLock()
        self.manual=None
        if MQTT_FILE.is_file():
            try:
                value=json.loads(MQTT_FILE.read_text(encoding='utf-8'))
                if isinstance(value,dict): self.manual=value
            except (ValueError,OSError): pass

    def status(self):
        with self.lock:
            custom=self.manual.copy() if self.manual else None
        if custom:
            return {'source':'manual', 'host':custom['host'],'port':custom['port'], 'username':custom.get('username',''),
                    'tls':custom.get('tls',False),'password_saved':bool(custom.get('password')),
                    'note':'Setting saved in the Builder; not yet written to RFLink'}
        try:
            data=ha_broker_service()
            return {'source':'home_assistant','host':data['host'],'port':data['port'],
                    'username':data.get('username',''),'tls':data['tls'], 'password_saved':bool(data.get('password')),
                    'note':'Detected automatically; the internal HA hostname may not be reachable from RFLink'}
        except Exception:
            return {'source':'none','host':'','port':1883,'username':'','tls':False,'password_saved':False,
                    'note':'No broker available; enter a LAN broker address'}

    def reveal_saved(self):
        with self.lock:
            # Never expose credentials supplied by Home Assistant Supervisor.
            return {'value': str(self.manual.get('password', '')) if self.manual else ''}

    def resolved(self):
        with self.lock:
            if self.manual: return self.manual.copy()
        return ha_broker_service()

    def save(self, body):
        if not isinstance(body,dict): raise ValueError('Invalid MQTT object')
        host=body.get('host','')
        if not isinstance(host,str) or not re.fullmatch(r'[A-Za-z0-9.:-]{1,253}',host):
            raise ValueError('Ongeldige MQTT-hostnaam of IP')
        try: port=int(body.get('port',1883))
        except (TypeError,ValueError) as exc: raise ValueError('Ongeldige MQTT-poort') from exc
        if not 1<=port<=65535: raise ValueError('MQTT-poort buiten bereik')
        username=body.get('username',''); password=body.get('password','')
        if not isinstance(username,str) or len(username)>128 or not isinstance(password,str) or len(password)>256:
            raise ValueError('MQTT username or password is too long')
        with self.lock:
            if not password and self.manual and self.manual.get('host')==host and self.manual.get('username')==username:
                password=self.manual.get('password','')
            self.manual={'host':host,'port':port,'username':username,'password':password,'tls':bool(body.get('tls',False))}
            DATA.mkdir(parents=True,exist_ok=True)
            temp=MQTT_FILE.with_suffix('.tmp')
            temp.write_text(json.dumps(self.manual),encoding='utf-8')
            temp.chmod(0o600)
            temp.replace(MQTT_FILE)
        return self.status()

    def reset(self):
        with self.lock:
            self.manual=None
            MQTT_FILE.unlink(missing_ok=True)
        return self.status()


mqtt_settings = MQTTSettings()


def push_mqtt_config(params: dict) -> dict:
    """Write broker configuration to a LAN RFLink native device (never log secrets).

    A running Radioactive firmware must expose the new MQTT management API.
    This does NOT depend on a successful build or on RFLink being paired.
    """
    if not isinstance(params, dict) or params.get('confirm') != 'CONFIGURE':
        raise ValueError('Type CONFIGURE to send broker settings to RFLink')
    ip = local_device_ip(params.get('ip', ''))
    key = params.get('key', '')
    if not isinstance(key, str) or not 12 <= len(key) <= 64:
        raise ValueError('Enter the RFLink administrator password')
    settings = mqtt_settings.resolved()
    if settings.get('tls'):
        raise ValueError('TLS settings cannot be sent to this test firmware yet; certificate validation is not implemented')
    # Names such as 'core-mosquitto' work inside the Supervisor network, but
    # do not resolve from a separately networked ESP32. Demand a LAN IPv4.
    host = settings.get('host', '')
    try:
        broker_ip = local_device_ip(host)
    except ValueError as exc:
        raise ValueError('Enter the broker LAN IPv4 address for MQTT, not the Home Assistant container hostname') from exc
    # LAN-only guard, no DNS redirects and no untrusted URLs.
    info = http_device_json(ip)
    if info.get('board_id') != 'rflink-s3-n16r8-reva' or not str(info.get('firmware', '')).startswith('0.7.0'):
        raise RuntimeError('Target is not compatible RFLink Radioactive N16R8 test firmware')
    if info.get('ota_busy') is True:
        raise RuntimeError('RFLink is bezig met OTA; configuratie geblokkeerd')
    opener = build_opener(ProxyHandler({}), NoRedirect())
    # Check API availability before sending passwords.
    with opener.open(Request(f'http://{ip}/api/mqtt/status', headers={'Accept': 'application/json'}), timeout=5) as rsp:
        status = json.loads(rsp.read(2048))
    if not isinstance(status, dict) or status.get('tls_supported') is not False:
        raise RuntimeError('RFLink does not support the expected MQTT configuration API')
    body = json.dumps({
        'host': broker_ip,
        'port': int(settings.get('port', 1883)),
        'username': str(settings.get('username', '')),
        'password': str(settings.get('password', '')),
        'tls': False,
    }).encode('utf-8')
    if len(body)>1200: raise ValueError('MQTT settings are too large for RFLink')
    request = Request(f'http://{ip}/api/mqtt/config', data=body, method='POST', headers={
        'Content-Type':'application/json','X-RFLink-Update-Key':key})
    with opener.open(request, timeout=12) as rsp:
        result = json.loads(rsp.read(2048))
    if result.get('ok') is not True or result.get('restart_required') is not True:
        raise RuntimeError('RFLink did not confirm saving the settings')
    return {'ok':True,'message':'Broker settings sent to RFLink. RFLink will restart and try to connect.',
            'rf_ip':ip,'broker':broker_ip,'restart_required':True}


class Deployment:
    def __init__(self):
        self.lock = threading.RLock()
        self.status='idle'; self.kind=''; self.message=''; self.error=''; self.started=''; self.finished=''
        self.device_id=None
        self.phase='idle'; self.verified=False
        self.target_version='';self.running_version=''
        self.ota_slot=None;self.ota_state=None
        self.upload_bytes=0;self.upload_total=0
        self.log_path=DATA/'deployment.log'
        self.process=None

    def snapshot(self):
        with self.lock:
            return {'status':self.status,'kind':self.kind,'message':self.message,'error':self.error,
                    'started_at':self.started,'finished_at':self.finished,'device_id':self.device_id,
                    'phase':self.phase,'verified':self.verified,'target_version':self.target_version,
                    'running_version':self.running_version,'ota_slot':self.ota_slot,
                    'ota_state':self.ota_state,'upload_bytes':self.upload_bytes,
                    'upload_total':self.upload_total}

    def start(self, kind, params):
        with builder.lock, self.lock:
            if builder.status!='success' or not builder.bundle or not builder.ota:
                raise RuntimeError('Complete a real successful ESP-IDF build first')
            if self.status=='running': raise RuntimeError('An installation is already running')
            manifest=json.loads(Path(builder.manifest).read_text(encoding='utf-8'))
            if manifest.get('chip')!='esp32s3' or manifest.get('test_only') is not True:
                raise RuntimeError('Only the explicit ESP32-S3 test manifest is allowed')
            target_device_id = manifest.get('builder_device_id')
            if target_device_id:
                if params.get('device_id') != target_device_id:
                    raise RuntimeError('The selected build belongs to a different RFLink device')
                config=device_book.get_config(target_device_id)
                if manifest.get('device_config_revision') != config.get('config_revision'):
                    raise RuntimeError('Device configuration changed after the build; rebuild before installing')
                require_selected_build(manifest, config.get('firmware_choice', 'current'))
                if kind == 'ota' and not (config.get('provisioned') and config.get('physical_id') and config.get('ip')):
                    raise RuntimeError('First installation must use USB; link the discovered RFLink before OTA')
            profile = device_book.get_config(target_device_id)['board'] if target_device_id else local_profile.current()
            if manifest.get('board_profile_hash') != local_profile._validate(profile):
                raise RuntimeError('Board settings changed since compilation; rebuild this version before installing')
            if (manifest.get('flash_size_bytes') != 16*1024*1024 or
                manifest.get('ota_slot_size_bytes') != 0x400000 or
                manifest.get('ota_0_offset') != 0x20000 or
                manifest.get('otadata_offset') != 0x19000):
                raise RuntimeError('Older release uses a different partition table; automatic installation is blocked')
            for key,meta in manifest['files'].items():
                target=Path(builder.manifest).parent/meta['filename']
                if not target.is_file() or hashlib.sha256(target.read_bytes()).hexdigest()!=meta['sha256']:
                    raise RuntimeError(f'Release file {key} failed SHA-256 verification')
            if params.get('confirm')!='TESTBOARD':
                raise ValueError('Type TESTBOARD to confirm; installation is restricted to a separate test board')
            self.status='running';self.kind=kind;self.message='Preparing verified firmware';self.error='';self.started=utc_now();self.finished=''
            self.phase='preflight';self.verified=False
            self.target_version=str(manifest.get('version') or '')[:100]
            self.running_version='';self.ota_slot=None;self.ota_state=None
            self.upload_bytes=0;self.upload_total=0
            self.device_id=params.get('device_id') if kind == 'ota' or target_device_id else None
            self.log_path.write_text(f'RFLink {kind} start {self.started}\n',encoding='utf-8')
            thread=threading.Thread(target=self._run,args=(kind,dict(params),manifest,Path(builder.manifest).parent),daemon=True)
            thread.start()
            return self.snapshot()

    def _run(self,kind,params,manifest,release):
        try:
            if kind=='ota': self._ota(params,manifest,release)
            elif kind=='usb': self._usb(params,manifest,release)
            else: raise ValueError('Onbekende installatiemethode')
            with self.lock:
                self.status='success'
                if kind=='ota':
                    self.message='New firmware booted and passed OTA health validation'
                    self.phase='verified'
                else:
                    self.message='USB flash command completed; hardware boot not verified'
                    self.phase='written'
        except OTAUnverified as exc:
            with self.lock:
                self.status='unverified';self.error=str(exc)
                self.message='OTA accepted but new firmware could not be confirmed'
                self.phase='unverified'
            with self.log_path.open('a',encoding='utf-8') as log:
                log.write('NOT VERIFIED: '+str(exc)+'\n')
        except OTARollback as exc:
            with self.lock:
                self.status='rolled_back';self.error=str(exc)
                self.message='ESP32 appears to have rolled back to an earlier firmware'
                self.phase='rolled_back'
            with self.log_path.open('a',encoding='utf-8') as log:
                log.write('ROLLBACK: '+str(exc)+'\n')
        except Exception as exc:
            with self.lock:
                self.status='failed';self.error=str(exc);self.message='Installation failed'
                self.phase='failed'
            with self.log_path.open('a',encoding='utf-8') as log: log.write('FAIL '+str(exc)+'\n')
        finally:
            with self.lock:self.finished=utc_now();self.process=None

    def _ota(self,params,manifest,release):
        selected_id = params.get('device_id', '')
        ip, key = device_book.get_target(selected_id)
        if not 12 <= len(key) <= 64:
            raise ValueError('Save the OTA key for the selected RFLink in Settings')
        status=http_device_json(ip)
        # Prevent a saved password being sent to a different board after DHCP
        # address reuse. Legacy firmware is always IP-bound in local storage.
        physical_id=device_book.devices.get(selected_id,{}).get('physical_id',selected_id)
        if physical_id.startswith('rf-') and status.get('device_id') != physical_id:
            raise RuntimeError('RFLink identity changed at this IP; OTA blocked')
        if status.get('board_id')!=manifest.get('board_id') or status.get('board_hash')!=manifest.get('board_profile_hash'):
            raise RuntimeError('RFLink board profile does not match; OTA blocked')
        if status.get('ota_busy') is True:
            raise RuntimeError('RFLink is already processing an OTA update')
        image=release/manifest['files']['app_ota']['filename']
        target_version=str(manifest.get('version') or '')
        if not target_version:
            raise ValueError('The firmware manifest has no verifiable embedded app version')
        binary=image.read_bytes()
        with self.log_path.open('a',encoding='utf-8') as log:
            log.write('[1/4] Preflight: device '+str(ip)+' · firmware '+str(status.get('firmware'))+'\n')
            log.write('       ESP32 identity: '+str(status.get('device_id') or 'legacy IP-bound')+
                      ' · board '+str(status.get('board_id'))+' · profile '+str(status.get('board_hash'))+'\n')
            log.write('       Active OTA slot: '+str(status.get('ota_slot'))+
                      ' · OTA health state: '+str(status.get('ota_state'))+'\n')
            log.write('       Target firmware: '+target_version+'\n')
            log.write('       Firmware file: '+str(len(binary))+' bytes · SHA-256 '+
                      str(manifest['files']['app_ota']['sha256'])[:16]+'…\n')
        # HTTP is only safe on a trusted isolated LAN. TLS must be added before production use.
        with self.lock:
            self.phase='uploading';self.message='Sending firmware to RFLink via LAN'
            self.upload_bytes=0;self.upload_total=len(binary)
        with self.log_path.open('a',encoding='utf-8') as log:
            log.write('[2/4] Uploading firmware over local HTTP/OTA: 0/'+str(len(binary))+' bytes (0%)\n')
        last_logged=[0]
        def upload_progress(sent,total):
            percent=100*sent//total
            with self.lock:
                self.upload_bytes=sent
                self.message=f'OTA transfer: {sent}/{total} bytes ({percent}%)'
            if percent>=last_logged[0]+5 or sent==total:
                last_logged[0]=percent
                with self.log_path.open('a',encoding='utf-8') as log:
                    log.write(f'       Sending at {sent}/{total} bytes ({percent}%)\n')
        def upload_sent():
            with self.lock:
                self.phase='awaiting_ack'
                self.message='All firmware bytes sent; waiting for ESP32 flash acknowledgement'
        result=send_ota_firmware(ip,key,binary,upload_progress,upload_sent)
        with self.log_path.open('a',encoding='utf-8') as log:
            log.write('       RFLink acknowledged OTA image: '+result.replace('\r',' ').replace('\n',' ')[:180]+'\n')
            log.write('[3/4] OTA written. Waiting for Wi-Fi reconnect and a different boot slot…\n')
        with self.lock:
            self.phase='accepted';self.message='OTA image accepted; waiting for ESP32 reboot'
        def report(phase,message,observation):
            with self.lock:
                self.phase=phase
                self.message=message
                if observation:
                    self.running_version=str(observation.get('firmware') or '')[:100]
                    self.ota_slot=observation.get('slot')
                    self.ota_state=observation.get('ota_state')
            with self.log_path.open('a',encoding='utf-8') as log:
                log.write('       '+message+'\n')
                if observation:
                    log.write('       Running: '+str(observation.get('firmware'))+
                              ' · slot '+str(observation.get('slot'))+
                              ' · OTA state '+str(observation.get('ota_state'))+
                              ' · uptime '+str(observation.get('uptime_s'))+'s\n')
        observed=confirm_ota_restart(lambda:http_device_json(ip),status,target_version,report)
        if device_book.migrate_verified_ota_identity(selected_id,observed.get('device_id'),target_version):
            with self.log_path.open('a',encoding='utf-8') as log:
                log.write('       Physical ID confirmed and saved for this RFLink after OTA health check.\n')
        with self.lock:
            self.verified=True
            self.running_version=str(observed.get('firmware') or '')[:100]
            self.ota_slot=observed.get('slot');self.ota_state=observed.get('ota_state')
        with self.log_path.open('a',encoding='utf-8') as log:
            log.write('[4/4] VERIFIED: '+target_version+
                      ' is running in the new OTA slot and health=VALID.\n')
            log.write('       This verifies the firmware boot and its basic health check, not RF on-air performance.\n')

    def _usb(self,params,manifest,release):
        device=valid_serial_port(params.get('port',''))
        if not shutil.which('python3'): raise RuntimeError('Python for esptool is missing')
        checks=[('bootloader',manifest['bootloader_offset']),('partition_table',manifest['partition_table_offset']),
                ('ota_data_initial',manifest['otadata_offset']),('app_ota',manifest['ota_0_offset'])]
        command=['python3','-m','esptool','--chip','esp32s3','--port',device,'--baud','460800',
                 'write-flash','--flash-size','16MB']
        for key,offset in checks:
            command.extend([hex(offset),str(release/manifest['files'][key]['filename'])])
        if manifest.get('builder_device_id'):
            nvs=manifest['files'].get('provisioning_nvs')
            if not isinstance(nvs,dict) or manifest.get('nvs_offset') != NVS_OFFSET or nvs.get('bytes') != NVS_SIZE:
                raise RuntimeError('Missing or incorrect device-specific Wi-Fi/OTA provisioning image')
            command.extend([hex(NVS_OFFSET), str(release/nvs['filename'])])
        # Read the immutable efuse MAC from this exact USB port before writing.
        if not toolchain.is_ready():
            raise RuntimeError('Install ESP-IDF toolchain before USB flashing')
        query=['python3','-m','esptool','--chip','esp32s3','--port',device,
               '--baud','115200','read-mac']
        result=subprocess.run(query,env=toolchain.environment(),capture_output=True,
                              text=True,timeout=60)
        hardware_id=esptool_hardware_id(result.stdout)
        if result.returncode!=0 or not hardware_id:
            raise RuntimeError('Cannot read ESP32-S3 hardware MAC; no firmware has been written')
        config_id=manifest.get('builder_device_id')
        if config_id:
            config=device_book.get_config(config_id)
            current=str(config.get('physical_id') or '')
            if current.startswith('rf-') and current!=hardware_id:
                raise RuntimeError('Wrong ESP32 selected: USB MAC differs from saved RFLink')
            for other in device_book.status()['devices']:
                if other.get('managed') and other.get('id')!=config_id and (
                    other.get('physical_id')==hardware_id or
                    other.get('expected_physical_id')==hardware_id):
                    raise RuntimeError('This physical ESP32 is assigned to another RFLink')
        with self.log_path.open('a',encoding='utf-8') as log:
            log.write('Connected physical ESP32: '+hardware_id+'\n')
        with self.lock:self.message=f'USB flashen op {device}'
        with self.log_path.open('a',encoding='utf-8') as log:
            log.write('USB esptool start voor ESP32-S3 N16R8\n')
            if not toolchain.is_ready():
                raise RuntimeError('Install ESP-IDF toolchain before USB flashing')
            proc=subprocess.Popen(command,env=toolchain.environment(),stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
            with self.lock:self.process=proc
            try: code=proc.wait(timeout=240)
            except subprocess.TimeoutExpired:
                os.killpg(proc.pid,signal.SIGKILL);proc.wait()
                raise RuntimeError('USB-flashen overschreed 240 seconden')
            if code!=0: raise RuntimeError(f'ESP-IDF USB flash failed (code {code})')
            log.write('USB flash command succeeded; hardware is not yet tested\n')
            if config_id:
                version=str(manifest.get('version') or '')
                if re.match(r'^0\.7\.0-exp\.[12]-', version):
                    # Published exp.1/exp.2 do not expose efuse MAC in /api/status.
                    device_book.mark_provisioned(config_id)
                    log.write('This Experimental release cannot report the hardware ID. '+
                              'Use manual Link after reboot, or flash exp.3 or newer.\n')
                else:
                    device_book.expect_flashing_hardware(config_id,hardware_id,version)
                    log.write('Auto-link pending: '+hardware_id+
                              ' · waiting for RFLink to return online with expected firmware.\n')

    def read_log(self):
        try:return self.log_path.read_text(encoding='utf-8',errors='replace')[-20000:]
        except FileNotFoundError:return ''


deployment = Deployment()



def prepare_build_workspace(workspace, pinned=None, config=None):
    """Stage an immutable release or current source in an isolated directory.

    The locally installed project, secrets and board-profile file are not
    modified by choosing an older or newer GitHub Release.
    """
    workspace = Path(workspace)
    if workspace.exists():
        shutil.rmtree(workspace)
    if pinned:
        raw = release_history.source(pinned)
        zf, entries = inspect_zip(raw)
        workspace.mkdir(parents=True)
        try:
            for info, rel in entries:
                dst = workspace.joinpath(*PurePosixPath(rel).parts)
                dst.parent.mkdir(parents=True, exist_ok=True)
                with zf.open(info) as inp, dst.open('wb') as out:
                    shutil.copyfileobj(inp, out, 64 * 1024)
        finally:
            zf.close()
    else:
        shutil.copytree(PROJECT,workspace,
                        ignore=shutil.ignore_patterns('build','release','sdkconfig',
                                                       'sdkconfig.old','managed_components',
                                                       '__pycache__'))
    # Overrides are only applied to the disposable, unpublished build workspace.
    if config:
        board = config['board']
        local_profile._validate(board)
        (workspace/'boards/rflink_s3_n16r8.json').write_text(json.dumps(board,indent=2)+'\n',encoding='utf-8')
    else:
        local_profile.apply_to_build(workspace)
    return workspace

class Builder:
    def __init__(self):
        self.lock = threading.RLock()
        self.status = 'idle'
        self.source: dict | None = None
        self.started_at: str | None = None
        self.finished_at: str | None = None
        self.error = ''
        self.process: subprocess.Popen | None = None
        self.job_id = ''
        self.bundle = ''
        self.ota = ''
        self.manifest = ''
        self.log_path = DATA / 'build.log'
        self.build_release = None
        self.device_id = None
        DATA.mkdir(parents=True, exist_ok=True)
        RELEASES.mkdir(parents=True, exist_ok=True)
        if PROJECT.exists() and (PROJECT / 'CMakeLists.txt').is_file():
            self.source = {'name': 'Existing project', 'uploaded_at': 'previously saved'}
        elif DEFAULT_SOURCE.is_file():
            data = DEFAULT_SOURCE.read_bytes()
            self.source = install_source(data, 'Meegeleverd: RFLink Radioactive Stage 20')

    def get_status(self):
        with self.lock:
            return {'status': self.status, 'source': self.source, 'started_at': self.started_at,
                    'finished_at': self.finished_at, 'error': self.error,
                    'job_id': self.job_id, 'ota_ready': bool(self.ota),
                    'bundle_ready': bool(self.bundle),
                    'idf_version': '5.5.6', 'board': 'ESP32-S3 N16R8',
                    'dev_mode': DEV_MODE, 'deploy': deployment.snapshot(),
                    'build_release': self.build_release, 'device_id': self.device_id}

    def upload(self, data: bytes, filename: str):
        with self.lock:
            if self.status in ('running', 'cancelling'):
                raise RuntimeError('A build is running; source changes are not allowed now')
            if deployment.snapshot()['status']=='running':
                raise RuntimeError('Source cannot be changed while firmware installation is running')
            self.source = install_source(data, filename)
            self.status = 'idle'
            self.error = ''
            self.bundle = self.ota = self.manifest = ''
            self.started_at = self.finished_at = None
            self.log_path.write_text('New source uploaded.\n', encoding='utf-8')
            return self.source

    def start(self, pinned=None, device_id=None):
        config = device_book.get_config(device_id) if device_id else None
        with self.lock:
            if self.status in ('running', 'cancelling'):
                raise RuntimeError('A build is already running')
            if deployment.snapshot()['status']=='running':
                raise RuntimeError('A new build cannot start while firmware installation is running')
            if not pinned and (not self.source or not PROJECT.is_dir()):
                raise ValueError('Upload a source ZIP first')
            self.status = 'running'
            self.build_release = dict(pinned) if pinned else None
            self.device_id = device_id
            self.error = ''
            self.bundle = self.ota = self.manifest = ''
            self.started_at = utc_now()
            self.finished_at = None
            self.job_id = uuid.uuid4().hex[:12]
            memory_monitor.start_build(self.job_id)
            self.log_path.write_text('RFLink Builder — nieuwe build\n', encoding='utf-8')
            thread = threading.Thread(target=self._run, args=(self.build_release, config), name='idf-build', daemon=True)
            thread.start()
            return self.job_id

    def cancel(self):
        with self.lock:
            if self.status != 'running':
                raise RuntimeError('No build is running')
            self.status = 'cancelling'
            proc = self.process
        if proc and proc.poll() is None:
            try:
                os.killpg(proc.pid, signal.SIGTERM)
            except (OSError, ProcessLookupError):
                pass

    def _run(self, pinned=None, config=None):
        job = self.job_id
        outcome = 'failed'
        steps = [
            ['python3', 'tools/generate_board_header.py'],
            ['python3', 'tools/preflight.py'],
            ['python3', 'tools/source_manifest.py'],
            ['idf.py', 'build'],
            ['python3', 'tools/package_release.py'],
        ]
        env = dict(os.environ)
        # Release identity must come only from a verified, SHA-pinned GitHub Release,
        # never an inherited add-on environment variable or a moving branch.
        env.pop('RFLINK_RELEASE_TAG', None)
        if pinned:
            env['RFLINK_RELEASE_TAG'] = pinned['tag']
        env.update({'IDF_COMPONENT_MANAGER': '1', 'PYTHONUNBUFFERED': '1'})
        try:
            if not toolchain.is_ready():
                raise RuntimeError('ESP-IDF is still installing; follow the setup console before building')
            env.update(toolchain.environment())
            workspace = prepare_build_workspace(DATA / 'build_workspace', pinned, config)
            deadline = time.monotonic() + MAX_BUILD_SECONDS
            with self.log_path.open('a', encoding='utf-8', errors='replace') as log:
                for step in steps:
                    if time.monotonic() >= deadline:
                        raise RuntimeError('Build-time-out na 55 minuten')
                    with self.lock:
                        if self.status == 'cancelling':
                            raise RuntimeError('Build afgebroken')
                    log.write('\n$ ' + ' '.join(step) + '\n')
                    log.flush()
                    proc = subprocess.Popen(step, cwd=workspace, env=env,
                                            stdout=log, stderr=subprocess.STDOUT,
                                            start_new_session=True)
                    with self.lock:
                        self.process = proc
                    try:
                        rc = proc.wait(timeout=max(1, deadline-time.monotonic()))
                    except subprocess.TimeoutExpired:
                        os.killpg(proc.pid, signal.SIGKILL)
                        proc.wait()
                        raise RuntimeError('Build-time-out na 55 minuten')
                    finally:
                        with self.lock:
                            self.process = None
                    with self.lock:
                        if self.status == 'cancelling':
                            raise RuntimeError('Build afgebroken')
                    if rc != 0:
                        raise RuntimeError('Build step failed: ' + ' '.join(step))
                source_release = workspace / 'release'
                manifest = source_release / 'release_manifest.json'
                if not manifest.is_file():
                    raise RuntimeError('Release package is missing despite a successful build')
                built = json.loads(manifest.read_text(encoding='utf-8'))
                if built.get('test_only') is not True or built.get('chip') != 'esp32s3':
                    raise RuntimeError('Release manifest contains an unexpected hardware status')
                if pinned and built.get('release_tag') != pinned['tag']:
                    raise RuntimeError('Built firmware version does not match selected immutable GitHub Release')
                if not pinned and built.get('release_tag') is not None:
                    raise RuntimeError('Local development source was unexpectedly stamped as a published release')
                # Persist exact source commit in the *installable* manifest, not
                # just in mutable Builder state or an optional build-info file.
                built['source_commit_sha'] = pinned['sha'] if pinned else None
                if pinned:
                    require_selected_build(built, pinned)
                target = RELEASES / job
                target.mkdir(parents=True, exist_ok=False)
                for f in source_release.iterdir():
                    if f.is_file():
                        shutil.copy2(f, target / f.name)
                if config:
                    nvs_target=target/'RFLink_device_provisioning_NVS.bin'
                    image_meta=create_nvs_image(config,nvs_target,Path(env['IDF_PATH']),env=env)
                    built['files']['provisioning_nvs']=image_meta
                    built['builder_device_id']=config['id']
                    built['device_config_revision']=config['config_revision']
                    built['nvs_offset']=NVS_OFFSET
                    built['nvs_size_bytes']=NVS_SIZE
                    (target/'PRIVATE_CREDENTIALS_README.txt').write_text(
                        'PRIVATE TESTBOARD package: NVS contains Wi-Fi and OTA secrets in cleartext.\n'
                        'Keep this ZIP secret; never upload it to GitHub. Only flash this partition\n'
                        'to a blank separate test ESP32-S3 N16R8 during the first USB installation.\n',encoding='utf-8')
                # Includes the immutable release SHA for every installation method.
                (target/'release_manifest.json').write_text(json.dumps(built,indent=2)+'\n',encoding='utf-8')
                appmeta = built.get('files', {}).get('app_ota', {})
                ota_name = appmeta.get('filename', '')
                ota_path = target / ota_name
                if ota_path.parent != target or not ota_path.is_file():
                    raise RuntimeError('OTA file is missing')
                with zipfile.ZipFile(target / 'RFLink_Radioactive_USB_OTA.zip', 'w', zipfile.ZIP_DEFLATED) as bundle:
                    for f in target.iterdir():
                        if f.is_file() and f.suffix != '.zip':
                            bundle.write(f, arcname=f.name)
                meta = {'job_id': job, 'source': 'github_release' if pinned else 'current_project',
                        'release_tag': pinned['tag'] if pinned else None,
                        'source_sha': pinned['sha'] if pinned else None,
                        'repo': pinned['repo'] if pinned else None,
                        'board_id': built.get('board_id'),
                        'board_profile_hash': built.get('board_profile_hash'),
                        'device_id': config['id'] if config else None,
                        'created_at': utc_now()}
                (target / 'build_info.json').write_text(json.dumps(meta, indent=2) + '\n', encoding='utf-8')
                with self.lock:
                    self.bundle = str(target / 'RFLink_Radioactive_USB_OTA.zip')
                    self.ota = str(ota_path)
                    self.manifest = str(target / 'release_manifest.json')
                # Keep the job running until disposable files have been removed.
                # Otherwise a second build can race with cleanup of this workspace.
                outcome = 'success'
                with self.log_path.open('a') as log:
                    log.write('\nPASS: real ESP-IDF build and release verification succeeded. NOT hardware tested.\n')
        except Exception as exc:
            with self.lock:
                outcome = 'canceled' if self.status == 'cancelling' else 'failed'
                self.error = str(exc)
            with self.log_path.open('a', encoding='utf-8') as log:
                log.write('\nFAIL: ' + str(exc) + '\n')
        finally:
            # This workspace is recreated on every build. Clearing it also
            # releases cache pages for deleted compiler outputs, where possible.
            # Firmware releases and the ESP-IDF toolchain are outside this path.
            try:
                discarded = discard_build_workspace(DATA)
                cleanup_note = ('Removed disposable build workspace; saved firmware and toolchain preserved.\n'
                                if discarded else '')
            except OSError as exc:
                # Cleanup is a best-effort resource optimization, not a reason
                # to invalidate a successfully verified firmware build.
                cleanup_note = 'WARNING: Build workspace cleanup failed: ' + str(exc)[:250] + '\n'
            if cleanup_note:
                try:
                    with self.log_path.open('a', encoding='utf-8') as log:
                        log.write(cleanup_note)
                except OSError:
                    pass
            with self.lock:
                self.process = None
                self.finished_at = utc_now()
                self.status = 'canceled' if self.status == 'cancelling' else outcome
            memory_monitor.finish_build(job)


    def saved_builds(self):
        """List only valid local build records; no caller-controlled paths."""
        builds=[]
        for metadata in RELEASES.glob('*/build_info.json'):
            if not re.fullmatch(r'[a-f0-9]{12}', metadata.parent.name):
                continue
            try:
                item=json.loads(metadata.read_text(encoding='utf-8'))
                if item.get('job_id')!=metadata.parent.name:
                    continue
                man=metadata.parent/'release_manifest.json'
                app=json.loads(man.read_text(encoding='utf-8'))
                if app.get('chip')!='esp32s3' or app.get('test_only') is not True:
                    continue
                if not (metadata.parent/'RFLink_Radioactive_USB_OTA.zip').is_file():
                    continue
                files=app.get('files',{})
                for fm in files.values():
                    if not isinstance(fm, dict) or not re.fullmatch(r'[a-zA-Z0-9._-]{1,120}',fm.get('filename','')):
                        raise ValueError('Invalid filename')
                    f=metadata.parent/fm['filename']
                    if not f.is_file() or hashlib.sha256(f.read_bytes()).hexdigest()!=fm.get('sha256'):
                        raise ValueError('Previously built file hash has changed')
                if not {'app_ota','bootloader','partition_table','ota_data_initial'}.issubset(files):
                    continue
                # A historical bundle can remain on disk for months; do not
                # offer it if any byte differs from its verified release set.
                with zipfile.ZipFile(metadata.parent/'RFLink_Radioactive_USB_OTA.zip') as bundle:
                    if bundle.testzip() is not None:
                        continue
                    names=bundle.namelist()
                    if len(names)!=len(set(names)):
                        continue
                    if any(fm['filename'] not in names or
                           hashlib.sha256(bundle.read(fm['filename'])).hexdigest()!=fm['sha256']
                           for fm in files.values()):
                        continue
                if item.get('release_tag') != app.get('release_tag'):
                    continue
                if app.get('release_tag') and item.get('source_sha') != app.get('source_commit_sha'):
                    continue
                builds.append({'job_id':item['job_id'],'release_tag':item.get('release_tag'),
                    'source':item.get('source'),'created_at':item.get('created_at'),
                    'board_profile_hash':app.get('board_profile_hash'),
                    'device_id':item.get('device_id'), 'chip':app.get('chip'),
                    'config_revision':app.get('device_config_revision'),
                    **build_identity(app, item)})
            except (OSError, ValueError, KeyError, TypeError, zipfile.BadZipFile):
                continue
        return sorted(builds,key=lambda r:r['created_at'] or '',reverse=True)[:50]

    def activate_build(self, job_id, device_id=None):
        if not isinstance(job_id,str) or not re.fullmatch(r'[a-f0-9]{12}',job_id):
            raise ValueError('Invalid build identifier')
        with self.lock:
            if self.status in ('running','cancelling') or deployment.snapshot()['status']=='running':
                raise RuntimeError('Build/installatie bezig')
            meta=next((b for b in self.saved_builds() if b['job_id']==job_id),None)
            if not meta:
                raise ValueError('No validated local build package is available for this version')
            # Match against the requested saved device, not global defaults.
            if meta.get('device_id') != device_id:
                raise RuntimeError('This build belongs to another device; select its exact configuration')
            profile = device_book.get_config(device_id)['board'] if device_id else local_profile.current()
            if meta['board_profile_hash'] != local_profile._validate(profile):
                raise RuntimeError('This firmware package has a different pinout/SPI profile hash. Rebuild with the current settings.')
            folder=RELEASES/job_id
            manifest=json.loads((folder/'release_manifest.json').read_text('utf-8'))
            if device_id:
                current_config=device_book.get_config(device_id)
                if manifest.get('device_config_revision')!=current_config.get('config_revision'):
                    raise RuntimeError('This saved build predates changes to the selected RFLink. Rebuild before using it.')
                chosen=current_config.get('firmware_choice','current')
                chosen_tag=chosen.get('tag') if isinstance(chosen,dict) else None
                require_selected_build(manifest, chosen, meta)
            self.bundle=str(folder/'RFLink_Radioactive_USB_OTA.zip')
            self.ota=str(folder/manifest['files']['app_ota']['filename'])
            self.manifest=str(folder/'release_manifest.json')
            self.status='success';self.error='';self.job_id=job_id
            self.device_id=device_id
            self.build_release={'tag':meta['release_tag']} if meta['release_tag'] else None
            self.started_at=meta['created_at'];self.finished_at=meta['created_at']
            self.log_path.write_text('Previously validated build selected: '+job_id+'\n',encoding='utf-8')
            return {'ok':True,'build':meta}

    def log_read(self, offset: int):
        try:
            with self.log_path.open('rb') as f:
                size = self.log_path.stat().st_size
                offset = min(max(0, offset), size)
                f.seek(offset)
                payload = f.read(MAX_LOG_CHUNK)
                return {'text': payload.decode('utf-8', errors='replace'),
                        'next_offset': offset + len(payload), 'size': size}
        except FileNotFoundError:
            return {'text': '', 'next_offset': 0, 'size': 0}


builder: Builder | None = None


def record_verified_usb_hardware(payload):
    """Enroll only the MAC read from a successful USB flash of this exact build.

    The browser has just flashed the physical COM port, so it can provide the
    efuse MAC before the device is visible on Wi-Fi. The Builder never assumes
    an online device from this report: it waits for the same hardware ID from
    the native /api/status endpoint.
    """
    config_id = payload.get('device_id')
    job_id = payload.get('job_id')
    identity = payload.get('hardware_id')
    if not isinstance(job_id, str) or not re.fullmatch(r'[a-f0-9]{12}', job_id):
        raise ValueError('Specify the successful USB build job')
    with builder.lock:
        if (builder.status != 'success' or builder.device_id != config_id or
            builder.job_id != job_id or not builder.manifest):
            raise RuntimeError('The active firmware build does not belong to this RFLink')
        config = device_book.get_config(config_id)
        manifest = json.loads(Path(builder.manifest).read_text('utf-8'))
        if (manifest.get('builder_device_id') != config_id or
            manifest.get('device_config_revision') != config.get('config_revision') or
            manifest.get('board_id') != config.get('board_id') or
            manifest.get('board_profile_hash') != config.get('board_hash')):
            raise RuntimeError('RFLink hardware or configuration changed after the USB build')
        require_selected_build(manifest, config.get('firmware_choice', 'current'))
        return device_book.expect_flashing_hardware(config_id, identity, str(manifest.get('version') or ''))


def prepared_browser_flash_image(payload):
    """Validate the exact device build before handing private bytes to the user's browser.

    Browser USB does not use any Home Assistant server USB port and never marks
    the device provisioned until hardware discovery confirms the installation.
    """
    device_id = payload.get('device_id')
    job_id = payload.get('job_id')
    if not isinstance(device_id, str) or not device_id or not re.fullmatch(r'[a-f0-9]{12}', str(job_id or '')):
        raise ValueError('Choose the configured RFLink and its exact validated build')
    if payload.get('confirm') != 'TESTBOARD':
        raise ValueError('Type TESTBOARD before preparing a first USB installation')
    with builder.lock:
        if deployment.snapshot()['status'] == 'running':
            raise RuntimeError('Another firmware installation is still running')
        if (builder.status != 'success' or builder.device_id != device_id
            or builder.job_id != job_id or not builder.manifest):
            raise RuntimeError('The matching per-device firmware build is no longer selected')
        config = device_book.get_config(device_id)
        if not config.get('configured'):
            raise RuntimeError('Browser USB requires a newly configured device with private Wi-Fi provisioning')
        manifest_path = Path(builder.manifest)
        manifest = json.loads(manifest_path.read_text(encoding='utf-8'))
        selected=config.get('firmware_choice','current')
        require_selected_build(manifest, selected)
        if (manifest.get('builder_device_id') != device_id
            or manifest.get('device_config_revision') != config['config_revision']
            or manifest.get('board_id') != config['board'].get('board_id')
            or manifest.get('board_profile_hash') != local_profile._validate(config['board'])):
            raise RuntimeError('Device configuration or pinout changed; rebuild firmware before USB flashing')
        if not toolchain.is_ready():
            raise RuntimeError('ESP-IDF toolchain is required to prepare a browser flash image')
        return browser_factory_image(manifest, manifest_path.parent, toolchain.environment())


class Handler(BaseHTTPRequestHandler):
    protocol_version = 'HTTP/1.1'
    # HA Ingress keeps HTTP/1.1 connections alive. Without a timeout every
    # idle connection retains a Python request thread indefinitely. This is
    # one plausible cause of elevated *process* RSS; actual savings still
    # need measuring on Home Assistant OS.
    idle_timeout_seconds = 20

    def handle_one_request(self):
        # do_POST permits 60s for upload; next idle request returns to 20s.
        self.connection.settimeout(self.idle_timeout_seconds)
        return super().handle_one_request()

    def log_message(self, fmt, *args):
        if fmt.startswith('Request timed out:'):
            return  # Normal, quiet cleanup of an idle Ingress keep-alive socket.
        if not self.path.startswith('/api/status'):
            print('[HTTP]', self.address_string(), fmt % args, flush=True)

    def _allowed(self):
        return DEV_MODE or self.client_address[0] == PROXY_IP

    def _send(self, status: int, body: bytes, kind='application/json; charset=utf-8',
              extra: dict | None = None):
        self.send_response(status)
        self.send_header('Content-Type', kind)
        self.send_header('Content-Length', str(len(body)))
        if self.close_connection:
            self.send_header('Connection', 'close')
        self.send_header('Cache-Control', 'no-store')
        self.send_header('X-Content-Type-Options', 'nosniff')
        self.send_header('Referrer-Policy', 'no-referrer')
        self.send_header('Content-Security-Policy', "default-src 'self'; script-src 'self'; style-src 'self'; connect-src 'self' blob:; worker-src 'self' blob:; object-src 'none'; base-uri 'self'")
        if extra:
            for name, value in extra.items():
                self.send_header(name, value)
        self.end_headers()
        try:
            self.wfile.write(body)
        except (BrokenPipeError, ConnectionResetError):
            pass

    def _json(self, status: int, value):
        self._send(status, json.dumps(value, ensure_ascii=False).encode('utf-8'))

    def _authorized(self, write=False):
        if not self._allowed():
            if write: self.close_connection = True
            self._json(403, {'error': 'Accessible only through Home Assistant Ingress'})
            return False
        if write and self.headers.get('X-RFLink-CSRF') != CSRF:
            self.close_connection = True
            self._json(403, {'error': 'Ongeldige sessiecode; ververs de pagina'})
            return False
        return True

    def do_GET(self):
        if not self._authorized():
            return
        path = urlparse(self.path).path
        if path in ('/', '/index.html'):
            data = (APP / 'static/index.html').read_bytes().replace(b'__RF_CSRF__', CSRF.encode('ascii'))
            return self._send(200, data, 'text/html; charset=utf-8')
        if path in ('/app.js', '/devices.js', '/wizard.js', '/device_firmware.js', '/firmware_catalog.js', '/browser_flash.js', '/rf_console.js', '/rf_flash.js', '/memory_diag.js', '/app.css', '/icon.png'):
            ext = ('image/png' if path.endswith('.png') else
                   'application/javascript; charset=utf-8' if path.endswith('.js') else
                   'text/css; charset=utf-8')
            return self._send(200, (APP / 'static' / path[1:]).read_bytes(), ext)
        if path == '/vendor/esptool-js/bundle.js':
            # Apache-2.0 esptool-js browser bundle pinned at image-build time.
            bundled=APP/'static/vendor/esptool-js/bundle.js'
            if not bundled.is_file():
                return self._json(503, {'error':'Native browser flasher not packaged; use legacy fallback'})
            return self._send(200,bundled.read_bytes(),'application/javascript; charset=utf-8')
        if path.startswith('/vendor/esp-web-tools/'):
            # Locally vendored, pinned 10.4.0 browser flasher. No CDN or
            # permissive cross-origin scripts: private NVS stays in-session.
            root=(APP / 'static/vendor/esp-web-tools').resolve()
            file=(APP / 'static' / path[1:]).resolve()
            kinds={'.js':'application/javascript; charset=utf-8',
                   '.css':'text/css; charset=utf-8',
                   '.wasm':'application/wasm',
                   '.json':'application/json; charset=utf-8',
                   '.svg':'image/svg+xml',
                   '.png':'image/png',
                   '.woff':'font/woff',
                   '.woff2':'font/woff2'}
            if file.is_relative_to(root) and file.is_file() and file.suffix in kinds:
                return self._send(200, file.read_bytes(), kinds[file.suffix])
            return self._json(404, {'error':'Browser USB asset not found'})
        if path == '/api/status':
            return self._json(200, builder.get_status())
        if path in ('/api/diagnostics/device', '/api/diagnostics/device/crashes',
                    '/api/diagnostics/device/console'):
            query=parse_qs(urlparse(self.path).query)
            device_id=query.get('device_id',[''])[0]
            try:
                if path.endswith('/crashes'):
                    return self._json(200,crash_report_for_device(device_book,device_id))
                if path.endswith('/console'):
                    try:
                        since=int(query.get('since',['0'])[0])
                    except (ValueError,TypeError):
                        raise ValueError('Invalid console cursor')
                    return self._json(200,console_for_device(device_book,device_id,since))
                stream=query.get('stream',['all'])[0]
                return self._json(200,diagnostics_for_device(device_book,device_id,stream))
            except ValueError as exc:
                return self._json(400,{'error':str(exc)})
            except (RuntimeError, URLError, HTTPError, OSError, json.JSONDecodeError) as exc:
                return self._json(502,{'error':'RFLink diagnosis unavailable: '+str(exc)[:180]})
        if path == '/api/diagnostics/memory':
            return self._json(200, memory_monitor.snapshot())
        if path == '/api/toolchain/status':
            return self._json(200, toolchain.snapshot())
        if path == '/api/toolchain/log':
            value = parse_qs(urlparse(self.path).query).get('offset', ['0'])[0]
            try:
                offset = max(0, int(value))
            except ValueError:
                offset = 0
            return self._json(200, toolchain.read_log(offset))
        if path == '/api/deploy/status':
            return self._json(200,deployment.snapshot())
        if path == '/api/deploy/log':
            return self._json(200,{'text':deployment.read_log()})
        if path == '/api/serial':
            return self._json(200,{'ports':serial_ports()})
        if path == '/api/mqtt':
            return self._json(200,mqtt_settings.status())
        if path == '/api/releases':
            repo=github_configuration().get('repo','')
            if not repo:
                return self._json(200, {'configured':False, 'recent':[], 'releases':[]})
            query=parse_qs(urlparse(self.path).query)
            try:
                scope=query.get('scope',['recent'])[0]
                if scope=='recent':
                    current=query.get('current',[''])[0]
                    return self._json(200, dict(release_history.recent(repo,current),configured=True))
                if scope=='channels':
                    return self._json(200, dict(release_history.channels(repo),configured=True))
                if scope=='all':
                    page=int(query.get('page',['1'])[0])
                    # Manual refresh is deliberately gated by the shared server
                    # cooldown; multiple browser tabs cannot spam GitHub.
                    force=query.get('refresh',['0'])[0]=='1'
                    if force and page!=1:
                        return self._json(400, {'error':'Only the first release page can be refreshed'})
                    return self._json(200, dict(release_history.page(repo,page,refresh=force),configured=True))
                return self._json(400, {'error':'Ongeldige releaseweergave'})
            except (ValueError, URLError, HTTPError, OSError) as e:
                return self._json(502, {'error':'GitHub Releases lookup failed: '+str(e)[:150]})
        if path == '/api/releases/built':
            return self._json(200, {'builds':builder.saved_builds()})
        if path == '/api/github':
            return self._json(200,{'repo':github_configuration().get('repo','')})
        if path == '/api/secrets':
            return self._json(200,local_secrets.status())
        if path == '/api/devices':
            return self._json(200, device_book.status())
        if path == '/api/devices/defaults':
            defaults=local_secrets.status()
            return self._json(200, {'board':local_profile.current(),
                'wifi_ssid':defaults['wifi_ssid'], 'wifi_password_saved':defaults['wifi_password_set'],
                'ota_key_format':'unique random 192-bit password'})
        if path == '/api/devices/config':
            ident=parse_qs(urlparse(self.path).query).get('id',[''])[0]
            try:return self._json(200,device_book.config_public(ident))
            except ValueError as exc:return self._json(400,{'error':str(exc)})
        if path == '/api/board':
            return self._json(200,{'board':local_profile.current(),'defaults':local_profile.defaults(),'local_override':local_profile.file.exists()})
        if path == '/api/log':
            try:
                offset = int(parse_qs(urlparse(self.path).query).get('offset', ['0'])[0])
            except ValueError:
                return self._json(400, {'error': 'Ongeldige logoffset'})
            return self._json(200, builder.log_read(offset))
        if path == '/api/download/device/usb':
            # The device page must never download a different RFLink's active
            # package, even if two browser sessions switch the global build.
            query=parse_qs(urlparse(self.path).query)
            device_id=query.get('device_id',[''])[0]
            job_id=query.get('job_id',[''])[0]
            if not isinstance(device_id,str) or not device_id or not re.fullmatch(r'[a-f0-9]{12}',job_id):
                return self._json(400,{'error':'Specify an existing device and validated build ID'})
            with builder.lock:
                device=device_book.devices.get(device_id)
                if not isinstance(device,dict) or not device.get('managed'):
                    return self._json(404,{'error':'RFLink device not found'})
                expected_id=device_id if device.get('configured') else None
                if (builder.status!='success' or builder.job_id!=job_id or
                    builder.device_id!=expected_id or not builder.manifest or not builder.bundle):
                    return self._json(409,{'error':'Another device build is selected. Select and verify this device again.'})
                manifest=json.loads(Path(builder.manifest).read_text('utf-8'))
                if (manifest.get('board_id')!=device.get('board_id') or
                    manifest.get('board_profile_hash')!=device.get('board_hash')):
                    return self._json(409,{'error':'Device hardware profile does not match this build'})
                if device.get('configured') and (
                    manifest.get('builder_device_id')!=device_id or
                    manifest.get('device_config_revision')!=device.get('config_revision')):
                    return self._json(409,{'error':'Device configuration changed. Rebuild before downloading.'})
                selection=device.get('firmware_choice','current')
                expected_tag=selection.get('tag') if isinstance(selection,dict) else None
                if device.get('configured'):
                    try: require_selected_build(manifest, selection)
                    except RuntimeError as exc:
                        return self._json(409, {'error':str(exc)})
                bundle_path=Path(builder.bundle)
                if not bundle_path.is_file():
                    return self._json(404,{'error':'Firmware package is missing'})
                contents=bundle_path.read_bytes()
            return self._send(200,contents,'application/zip',
                {'Content-Disposition':'attachment; filename="RFLink_USB_OTA.zip"'})
        files = {'/api/download/ota': ('ota', 'application/octet-stream'),
                 '/api/download/usb': ('bundle', 'application/zip'),
                 '/api/download/manifest': ('manifest', 'application/json')}
        if path in files:
            key, mime = files[path]
            with builder.lock:
                target = getattr(builder, key)
                if builder.status != 'success' or not target:
                    return self._json(404, {'error': 'No validated build is available yet'})
                file_path = Path(target)
                if builder.device_id:
                    try:
                        config=device_book.get_config(builder.device_id)
                        manifest=json.loads(Path(builder.manifest).read_text('utf-8'))
                        require_selected_build(manifest,config.get('firmware_choice','current'))
                    except (RuntimeError, ValueError, OSError) as exc:
                        return self._json(409,{'error':'Firmware selection changed: '+str(exc)})
            if not file_path.is_file():
                return self._json(404, {'error': 'Build file is missing'})
            contents = file_path.read_bytes()
            return self._send(200, contents, mime, {'Content-Disposition': 'attachment; filename="' + file_path.name + '"'})
        return self._json(404, {'error': 'Unknown path'})

    def do_POST(self):
        if not self._authorized(write=True):
            return
        path = urlparse(self.path).path
        if path not in ('/api/upload', '/api/build', '/api/cancel', '/api/deploy/ota', '/api/deploy/usb',
                        '/api/browser-flash/image',
                        '/api/mqtt/save', '/api/mqtt/test', '/api/mqtt/reset', '/api/mqtt/push', '/api/source/reset',
                        '/api/github/save','/api/github/check','/api/github/preview','/api/source/preview',
                        '/api/source/apply','/api/board/save','/api/board/reset',
                        '/api/secrets/save','/api/secrets/clear', '/api/build/release',
                        '/api/releases/use', '/api/releases/device', '/api/toolchain/retry',
                         '/api/devices/discover', '/api/devices/add', '/api/devices/network',
                         '/api/devices/adopt', '/api/devices/rename', '/api/devices/remove',
                         '/api/devices/create', '/api/devices/config/save', '/api/devices/firmware/select',
                         '/api/devices/key/generate', '/api/devices/link', '/api/devices/usb-flash/identify', '/api/build/device',
                         '/api/devices/wifi/reveal',
                         '/api/devices/key/save', '/api/devices/key/reveal',
                         '/api/devices/key/delete', '/api/secrets/reveal', '/api/mqtt/reveal'):
            # An unknown POST may have an unread chunked body.
            # Close rather than interpret its chunk sizes as HTTP requests.
            self.close_connection = True
            return self._json(404, {'error': 'Onbekende actie'})
        try:
            # Decode and consume the entire request before triggering any
            # side effect. Home Assistant Ingress can use chunked encoding
            # even for empty or tiny POST bodies.
            max_body = MAX_UPLOAD if path in ('/api/upload', '/api/source/preview') else MAX_JSON
            self.connection.settimeout(60)
            body = read_body(self.headers, self.rfile, max_body)
            length = len(body)
            if path == '/api/browser-flash/image':
                if not length or length > MAX_JSON or 'application/json' not in self.headers.get('Content-Type', ''):
                    return self._json(400, {'error': 'JSON first-install request required'})
                payload = json.loads(body)
                image = prepared_browser_flash_image(payload)
                return self._send(200, image, 'application/octet-stream',
                                  {'X-RFLink-Factory-Image': 'ESP32-S3-PRIVATE-TESTBOARD',
                                   'X-RFLink-Image-SHA256': hashlib.sha256(image).hexdigest()})
            if path == '/api/toolchain/retry':
                return self._json(202, toolchain.start())
            if path in ('/api/deploy/ota', '/api/deploy/usb', '/api/mqtt/save', '/api/mqtt/push',
                        '/api/github/save','/api/source/apply','/api/board/save','/api/secrets/save',
                        '/api/build/release','/api/releases/use','/api/releases/device',
                         '/api/devices/add','/api/devices/network','/api/devices/adopt',
                         '/api/devices/rename','/api/devices/remove','/api/devices/key/save',
                          '/api/devices/create','/api/devices/config/save','/api/devices/firmware/select',
                          '/api/devices/link','/api/devices/usb-flash/identify','/api/build/device',
                          '/api/devices/wifi/reveal',
                         '/api/devices/firmware/select',
                         '/api/devices/key/reveal','/api/devices/key/delete',
                         '/api/secrets/reveal','/api/mqtt/reveal'):
                if not length or length>MAX_JSON or 'application/json' not in self.headers.get('Content-Type',''):
                    return self._json(400,{'error':'JSON request required (max 8 KB)'})
                payload=json.loads(body)
                if path=='/api/devices/add':
                    return self._json(200, device_book.add(payload.get('ip', '')))
                if path=='/api/devices/create':
                    board=payload.get('board')
                    digest=local_profile._validate(board)
                    ssid=payload.get('wifi_ssid','')
                    password=payload.get('wifi_password','')
                    if payload.get('inherit_wifi_password') is True:
                        defaults=local_secrets.load()
                        if ssid != defaults.get('wifi_ssid'):
                            raise ValueError('Default Wi-Fi password only applies to the default SSID')
                        password=defaults.get('wifi_password','')
                    key=payload.get('ota_key','')
                    if not key:
                        key=device_book.generate_key()['key']
                    selected=payload.get('firmware','current')
                    if isinstance(selected,dict):
                        selected=release_history.resolve(github_configuration()['repo'],selected.get('tag'),selected.get('release_id'))
                    result=device_book.create_config(payload.get('name',''),board,digest,ssid,password,key,selected)
                    return self._json(200, {'config':result, 'ota_key':key})
                if path=='/api/devices/config/save':
                    board=payload.get('board')
                    digest=local_profile._validate(board)
                    return self._json(200,device_book.update_config(payload.get('device_id'),board,digest,
                                             payload.get('wifi_ssid'),payload.get('wifi_password','')))
                if path=='/api/devices/link':
                    return self._json(200,device_book.link(payload.get('config_id'),payload.get('discovered_id')))
                if path=='/api/devices/usb-flash/identify':
                    return self._json(200,record_verified_usb_hardware(payload))
                if path=='/api/devices/wifi/reveal':
                    return self._json(200,{'value':device_book.get_wifi_password(payload.get('device_id'))})
                if path=='/api/devices/firmware/select':
                    ident=payload.get('device_id')
                    selected=payload.get('firmware')
                    with builder.lock:
                        if builder.status in ('running','cancelling') or deployment.snapshot()['status']=='running':
                            raise RuntimeError('Wait until the current build or installation finishes before changing firmware')
                        # Always re-resolve user input through GitHub's published
                        # immutable Release ID and exact source commit SHA.
                        if selected == 'current':
                            verified='current'
                        elif isinstance(selected,dict):
                            repo=github_configuration().get('repo','')
                            if not repo: raise ValueError('Configure a GitHub repository to select released firmware')
                            verified=release_history.resolve(repo,selected.get('tag'),selected.get('release_id'))
                        else:
                            raise ValueError('Select a published firmware version or current development source')
                        return self._json(200,device_book.select_firmware(ident,verified))
                if path=='/api/build/device':
                    ident=payload.get('device_id')
                    config=device_book.get_config(ident)
                    selected=config.get('firmware_choice','current')
                    return self._json(202,{'ok':True,'job_id':builder.start(selected if isinstance(selected,dict) else None,ident)})
                if path=='/api/devices/adopt':
                    return self._json(200, device_book.adopt(payload.get('device_id', ''), payload.get('name', '')))
                if path=='/api/devices/rename':
                    return self._json(200, device_book.rename(payload.get('device_id', ''), payload.get('name', '')))
                if path=='/api/devices/remove':
                    return self._json(200, remove_device_and_private_builds(payload.get('device_id', '')))
                if path=='/api/devices/network':
                    return self._json(200, device_book.set_network(payload.get('network', '')))
                if path=='/api/devices/key/save':
                    return self._json(200, device_book.save_key(payload.get('device_id', ''), payload.get('key', '')))
                if path=='/api/devices/key/delete':
                    return self._json(200, device_book.delete_key(payload.get('device_id', '')))
                if path=='/api/devices/key/reveal':
                    return self._json(200, {'value': device_book.get_key(payload.get('device_id', ''))})
                if path=='/api/secrets/reveal':
                    return self._json(200, local_secrets.reveal(payload.get('field', '')))
                if path=='/api/mqtt/reveal':
                    return self._json(200, mqtt_settings.reveal_saved())
                if path=='/api/releases/device':
                    ip=local_device_ip(payload.get('ip',''))
                    status=http_device_json(ip)
                    # Native RFLink only; do not infer compatibility for ESPHome v0.6.x.
                    if not status.get('board_id') or not status.get('board_hash'):
                        raise ValueError('This device does not have the native Radioactive status API')
                    return self._json(200,{'version':str(status.get('firmware') or '')[:120],
                                           'board_id':status['board_id'],
                                           'board_hash':status['board_hash']})
                if path=='/api/releases/use':
                    return self._json(200,builder.activate_build(payload.get('job_id',''),payload.get('device_id')))
                if path=='/api/build/release':
                    repo=github_configuration().get('repo','')
                    if not repo: raise ValueError('Stel eerst je GitHub-repository in')
                    pinned=release_history.resolve(repo,payload.get('tag'),payload.get('release_id'))
                    return self._json(202,{'ok':True,'job_id':builder.start(pinned,payload.get('device_id'))})
                if path=='/api/github/save':
                    return self._json(200,save_github_repo(payload.get('repo','')))
                if path=='/api/source/apply':
                    with builder.lock:
                        if builder.status in ('running','cancelling') or deployment.snapshot()['status']=='running':
                            raise RuntimeError('Build/installatie bezig')
                        return self._json(200,apply_comparison(payload.get('token','')))
                if path=='/api/secrets/save':
                    return self._json(200,local_secrets.save(payload))
                if path=='/api/board/save':
                    return self._json(200,local_profile.save(payload.get('board')))
                if path=='/api/mqtt/push':
                    return self._json(200,push_mqtt_config(payload))
                if path=='/api/mqtt/save':
                    return self._json(200,{'ok':True,'settings':mqtt_settings.save(payload)})
                method=path.rsplit('/',1)[-1]
                return self._json(202,{'ok':True,'deployment':deployment.start(method,payload)})
            if path == '/api/devices/discover':
                return self._json(202, device_book.discover())
            if path == '/api/devices/refresh':
                # Refresh saved IPs only: never scan two /24 subnets when the
                # browser reloads or the user wants a quick health check.
                device_book.refresh_known()
                return self._json(202, device_book.status())
            if path == '/api/devices/key/generate':
                return self._json(200, device_book.generate_key())
            if path == '/api/github/check':
                return self._json(200,github_status())
            if path == '/api/github/preview':
                with builder.lock:
                    if builder.status in ('running','cancelling') or deployment.snapshot()['status']=='running':
                        raise RuntimeError('Build/installatie bezig')
                    return self._json(200,compare_github())
            if path == '/api/secrets/clear':
                return self._json(200,local_secrets.clear())
            if path == '/api/board/reset':
                return self._json(200,local_profile.reset())
            if path == '/api/source/reset':
                return self._json(200, {'ok':True,'source':builder.upload(DEFAULT_SOURCE.read_bytes(),'Meegeleverd: RFLink Radioactive Stage 20')})
            if path in ('/api/mqtt/test','/api/mqtt/reset'):
                if path.endswith('reset'):
                    return self._json(200,{'ok':True,'settings':mqtt_settings.reset()})
                return self._json(200,{'ok':True,'message':mqtt_check(mqtt_settings.resolved())})
            if path in ('/api/upload','/api/source/preview'):
                if self.headers.get('Content-Type', '').split(';')[0].strip() != 'application/zip':
                    return self._json(415, {'error': 'Only ZIP source archives are supported'})
                if length == 0:
                    return self._json(400, {'error': 'Empty file'})
                payload = body
                name = self.headers.get('X-RFLink-Filename', 'RFLink source.zip')
                if not name.lower().endswith('.zip'):
                    return self._json(400, {'error': 'Choose a ZIP file'})
                if path=='/api/source/preview':
                    with builder.lock:
                        if builder.status in ('running','cancelling') or deployment.snapshot()['status']=='running':
                            raise RuntimeError('Build/installatie bezig')
                        return self._json(200,compare_uploaded(payload))
                result = builder.upload(payload, name)
                return self._json(200, {'ok': True, 'source': result})
            if path == '/api/build':
                return self._json(202, {'ok': True, 'job_id': builder.start()})
            builder.cancel()
            return self._json(202, {'ok': True})
        except BodyReadError as exc:
            self.close_connection = True
            return self._json(exc.status, {'error': str(exc)})
        except (ValueError, zipfile.BadZipFile) as exc:
            return self._json(400, {'error': str(exc)})
        except RuntimeError as exc:
            return self._json(409, {'error': str(exc)})
        except (HTTPError, URLError, OSError) as exc:
            return self._json(502, {'error': 'Connection failed: ' + str(exc)[:160]})
        except Exception as exc:
            print('Action exception:', repr(exc), flush=True)
            return self._json(500, {'error': 'Internal error; check the app log'})


def main():
    global builder
    builder = Builder()
    memory_monitor.checkpoint("Builder initialized")
    httpd = ThreadingHTTPServer(('0.0.0.0', PORT), Handler)
    httpd.daemon_threads = True
    memory_monitor.checkpoint("HTTP server bound")
    def sample_memory():
        while True:
            try:
                build_state=builder.get_status()['status']
                deploy_state=deployment.snapshot()['status']
                tool_state=toolchain.snapshot()['status']
                if build_state in ('running','cancelling'):
                    phase='building'
                elif deploy_state=='running':
                    phase='installing'
                elif tool_state=='running':
                    phase='toolchain_setup'
                elif device_book.scanning or device_book.refreshing:
                    phase='scanning'
                else:
                    phase='idle'
                memory_monitor.sample(phase)
            except Exception as exc:
                print('Memory diagnostic sampling failed:',type(exc).__name__,flush=True)
            threading.Event().wait(5)
    threading.Thread(target=sample_memory,daemon=True,name='rflink-memory-monitor').start()
    if not DEV_MODE:
        toolchain.start()
        memory_monitor.checkpoint("Toolchain setup checked")
        if any(isinstance(d, dict) and d.get('expected_physical_id')
               for d in device_book.devices.values()):
            # After a Builder restart, a flashed board may need to be found
            # on its new DHCP address; only pending enrollment scans the LAN.
            device_book.discover()
        else:
            device_book.refresh_known()
        memory_monitor.checkpoint("Initial device health check started")
        def measure_initial_scan():
            # Only one short-lived watcher; do not add recurring scan threads.
            while device_book.scanning or device_book.refreshing:
                threading.Event().wait(0.25)
            memory_monitor.checkpoint("Initial device health check completed")
        threading.Thread(target=measure_initial_scan,daemon=True,name='rflink-first-scan-memory').start()
        def monitor_known_devices():
            while True:
                threading.Event().wait(60)
                device_book.refresh_known()
        threading.Thread(target=monitor_known_devices, daemon=True, name='rflink-device-monitor').start()
    print(f'RFLink Universal Builder v{os.environ.get("RFLINK_BUILDER_VERSION", "development")} listening on {PORT} ({"DEVELOPMENT" if DEV_MODE else "HA INGRESS ONLY"})', flush=True)
    print('ESP-IDF toolchain:', toolchain.snapshot()['status'], flush=True)
    try:
        httpd.serve_forever(poll_interval=0.4)
    except KeyboardInterrupt:
        httpd.server_close()


if __name__ == '__main__':
    main()
