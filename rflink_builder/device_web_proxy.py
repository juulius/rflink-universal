"""Trusted native RFLink web UI and identity-bound LAN API relay for HA Ingress.

Only vetted Builder-bundled HTML/JS/CSS is executed in the browser. Neither
device-supplied scripts nor arbitrary LAN URLs are ever forwarded.
"""
from __future__ import annotations
import re
import threading
import time
from pathlib import Path
from urllib.error import HTTPError, URLError
from urllib.parse import urlsplit
from urllib.request import Request

from devices import _probe_opener, local_ip, probe

_ID = re.compile(r'(?:rf-[0-9a-f]{12}|cfg-[0-9a-f]{18})\Z')
_ROUTE = re.compile(r'/rfweb/([^/]+)/?(.*)\Z')
_GET = frozenset((
    'api/status', 'api/diagnostics/crashes', 'api/diagnostics/console',
    'api/radio/capture', 'api/logs', 'api/logs/detail', 'api/logs.csv',
    'api/logs/config', 'api/led', 'api/events', 'api/events.csv',
    'api/mqtt/status',
))
_POST = frozenset((
    'api/logs/ui', 'api/logs/config', 'api/led', 'api/mqtt/config',
    'api/ota',
))
_STATIC = {'': 'index.html', 'index.html': 'index.html',
           'app.js': 'app.js', 'app.css': 'app.css'}
_TYPES = {'index.html': 'text/html; charset=utf-8',
          'app.js': 'application/javascript; charset=utf-8',
          'app.css': 'text/css; charset=utf-8'}
_MAX_REPLY = 6 * 1024 * 1024
_MAX_BODY = 4 * 1024 * 1024 + 1024
_VERIFIED = {}
_VERIFY_LOCK = threading.Lock()


class ProxyRequestError(ValueError):
    """A refused proxy request, with an HTTP code safe for the browser."""
    def __init__(self, status, message):
        super().__init__(message)
        self.status = status


def _parse(raw_path):
    parts = urlsplit(raw_path)
    match = _ROUTE.fullmatch(parts.path)
    if not match or not _ID.fullmatch(match.group(1)):
        raise ProxyRequestError(404, 'Unknown RFLink device route')
    device_id, resource = match.groups()
    if parts.query and (len(parts.query) > 512 or resource in _STATIC):
        raise ProxyRequestError(400, 'Invalid RFLink query')
    if '..' in resource or '\\' in resource or '//' in resource:
        raise ProxyRequestError(404, 'Unsupported RFLink path')
    return device_id, resource, parts.query


def _verified_ip(book, device_id, *, writing=False):
    with book.lock:
        saved = book.devices.get(device_id)
        if not isinstance(saved, dict) or not saved.get('managed') or not saved.get('online'):
            raise ProxyRequestError(409, 'This RFLink is not online or managed')
        ip = local_ip(saved.get('ip', ''))
        physical = saved.get('physical_id') or (
            device_id if device_id.startswith('rf-') else '')
        board, board_hash = saved.get('board_id'), saved.get('board_hash')
        if not (saved.get('identity_verified') and
                _ID.fullmatch(str(physical)) and physical.startswith('rf-') and
                isinstance(board, str) and isinstance(board_hash, str) and
                board.startswith('rflink-') and bool(board_hash)):
            raise ProxyRequestError(409, 'RFLink identity must be verified first')
    fingerprint = (device_id, ip, physical, board, board_hash)
    now = time.monotonic()
    with _VERIFY_LOCK:
        cached = _VERIFIED.get(device_id)
        if not writing and cached and cached[0] == fingerprint and now < cached[1]:
            return ip
    try:
        actual = probe(ip, timeout=2)
    except (ValueError, OSError, URLError, HTTPError) as exc:
        raise ProxyRequestError(502, 'RFLink identity check is unavailable') from exc
    if not actual or actual['id'] != physical or (
        actual.get('board_id') != board or actual.get('board_hash') != board_hash):
        raise ProxyRequestError(409, 'RFLink identity changed. Remote control is blocked.')
    with _VERIFY_LOCK:
        if len(_VERIFIED) > 64:
            _VERIFIED.clear()
        _VERIFIED[device_id] = (fingerprint, now + 5)
    return ip


def handle_request(book, raw_path, method, csrf, *, body=b'', headers=None,
                   asset_root=None):
    """Return (status, bytes, MIME). No redirects, cookies or device JS."""
    device_id, resource, query = _parse(raw_path)
    if method == 'GET' and resource in _STATIC:
        root = Path(asset_root) if asset_root else Path(__file__).resolve().parent / 'static/rflink_web'
        name = _STATIC[resource]
        content = (root / name).read_bytes()
        if name == 'index.html':
            content = content.replace(b'__RF_CSRF__', csrf.encode('ascii'))
        return 200, content, _TYPES[name]
    if method == 'GET':
        if resource not in _GET:
            raise ProxyRequestError(404, 'Unsupported RFLink read endpoint')
    elif method == 'POST':
        if resource not in _POST or query:
            raise ProxyRequestError(404, 'Unsupported RFLink write endpoint')
        if len(body) > _MAX_BODY:
            raise ProxyRequestError(413, 'RFLink request is too large')
    else:
        raise ProxyRequestError(405, 'Unsupported RFLink request method')
    ip = _verified_ip(book, device_id, writing=(method == 'POST'))
    upstream_headers = {'Accept': 'application/json'}
    if method == 'POST':
        content_type = (headers.get('Content-Type', '') if headers else '').split(';', 1)[0].strip().lower()
        if resource == 'api/ota':
            if content_type != 'application/octet-stream' or not body:
                raise ProxyRequestError(415, 'OTA requires a firmware binary')
            key = headers.get('X-RFLink-Update-Key', '') if headers else ''
            if not 12 <= len(key) <= 64:
                raise ProxyRequestError(400, 'Enter the RFLink OTA password')
            upstream_headers['X-RFLink-Update-Key'] = key
        elif content_type != 'application/json':
            raise ProxyRequestError(415, 'RFLink action requires JSON')
        upstream_headers['Content-Type'] = content_type
    target = 'http://' + ip + '/' + resource + ('?' + query if query else '')
    req = Request(target, data=body if method == 'POST' else None,
                  headers=upstream_headers, method=method)
    try:
        with _probe_opener().open(req, timeout=90 if resource == 'api/ota' else 4) as response:
            status, content = response.status, response.read(_MAX_REPLY + 1)
    except HTTPError as exc:
        status, content = exc.code, exc.read(_MAX_REPLY + 1)
    except (OSError, URLError) as exc:
        raise ProxyRequestError(502, 'RFLink is unreachable from Home Assistant') from exc
    if len(content) > _MAX_REPLY:
        raise ProxyRequestError(502, 'RFLink response exceeds the safe size limit')
    kind = ('text/csv; charset=utf-8' if resource.endswith('.csv') else
            'text/plain; charset=utf-8' if resource == 'api/ota' else
            'application/json; charset=utf-8')
    return status, content, kind
