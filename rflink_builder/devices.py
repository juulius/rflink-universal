"""RFLink LAN discovery and device-specific OTA credentials.

Only devices answering the native RFLink status API are listed. No network
hosts or secrets are sent to GitHub. Scan scope is limited to private /24s.
"""
from __future__ import annotations
from concurrent.futures import ThreadPoolExecutor, as_completed
import ipaddress
import json
import os
from pathlib import Path
import re
import threading
import secrets
import hashlib
from datetime import datetime, timezone
from urllib.request import Request, build_opener, ProxyHandler, HTTPRedirectHandler

# Two complete /24 scans were responsible for a >430 MB increase in Python
# RSS on the actual Home Assistant installation. Do not fan out 48 concurrent
# network clients: glibc arenas and thread allocations can remain resident
# even when a scan has finished. Keep enough concurrency for LAN discovery.
DISCOVERY_WORKERS = 12
HEALTH_WORKERS = 4
_probe_thread = threading.local()


def _probe_opener():
    """One no-proxy, no-redirect HTTP opener per short-lived scan worker.

    Reuse its handler objects for multiple addresses instead of allocating
    a fresh opener (including an HTTPS handler) for every single host.
    The opener is not shared across threads or retained after pool shutdown.
    """
    opener = getattr(_probe_thread, 'opener', None)
    if opener is None:
        opener = build_opener(ProxyHandler({}), NoRedirect())
        _probe_thread.opener = opener
    return opener

class NoRedirect(HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        return None

def local_ip(value):
    try:
        ip = ipaddress.IPv4Address(value)
    except (ipaddress.AddressValueError, ValueError) as exc:
        raise ValueError("Enter a private IPv4 address") from exc
    if not ip.is_private or ip.is_loopback or ip.is_link_local or ip.is_multicast:
        raise ValueError("Only private LAN IPv4 addresses are supported")
    if not any(ip in net for net in (ipaddress.ip_network("10.0.0.0/8"),
                                     ipaddress.ip_network("172.16.0.0/12"),
                                     ipaddress.ip_network("192.168.0.0/16"))):
        raise ValueError("Only RFC1918 LAN addresses are supported")
    return str(ip)

def scan_network(value):
    try:
        network = ipaddress.IPv4Network(value, strict=True)
    except (ValueError, ipaddress.AddressValueError) as exc:
        raise ValueError("Enter a private /24 network, e.g. 192.168.1.0/24") from exc
    if network.prefixlen != 24 or not all(local_ip(str(a)) for a in (network.network_address + 1, network.broadcast_address - 1)):
        raise ValueError("Only a private /24 network may be scanned")
    return str(network)

def probe(ip, timeout=0.6):
    ip = local_ip(ip)
    request = Request("http://" + ip + "/api/status", headers={"Accept": "application/json"})
    with _probe_opener().open(request, timeout=timeout) as response:
        content = response.read(8193)
    if len(content) > 8192:
        return None
    info = json.loads(content)
    if not isinstance(info, dict):
        return None
    board = info.get("board_id")
    version = info.get("firmware")
    fingerprint = info.get("board_hash")
    if not isinstance(board, str) or not board.startswith("rflink-") or not isinstance(version, str) or not isinstance(fingerprint, str):
        return None
    unique = info.get("device_id")
    if isinstance(unique, str) and re.fullmatch(r"rf-[0-9a-f]{12}", unique):
        key = unique
    else:
        # Older native firmware has no physical identity. Do NOT share credentials
        # across two identical board hashes; keep legacy passwords IP-bound.
        key = "ip:" + ip
    return {"id": key, "ip": ip, "name": "RFLink " + ip,
            "identity_verified": not key.startswith("ip:"),
            "firmware": version[:80], "board_id": board[:80],
            "board_hash": fingerprint[:80], "online": True,
            "wifi_connected": info.get("wifi_connected") is True}

class DeviceBook:
    def __init__(self, data):
        self.path = Path(data) / "local" / "devices.json"
        self.lock = threading.RLock()
        self.devices = {}
        self.network = ""
        self.scanning = False
        self.refreshing = False
        self.error = ""
        self._load()
        # Saved reachability is never trusted across process restarts.
        for item in self.devices.values():
            if isinstance(item, dict): item["online"] = False

    def _load(self):
        try:
            raw = json.loads(self.path.read_text("utf-8"))
            if isinstance(raw, dict):
                self.devices = raw.get("devices", {}) if isinstance(raw.get("devices"), dict) else {}
                # Compatibility with 0.6.0 device books: previously scanned
                # entries that already have an OTA key are managed entries.
                for item in self.devices.values():
                    if isinstance(item, dict) and "managed" not in item:
                        item["managed"] = bool(item.get("key"))
                cidr = raw.get("network", "")
                self.network = scan_network(cidr) if cidr else ""
        except (OSError, ValueError, TypeError):
            self.devices = {}

    def _persist(self):
        self.path.parent.mkdir(parents=True, exist_ok=True, mode=0o700)
        self.path.parent.chmod(0o700)
        tmp = self.path.with_suffix(".tmp")
        with tmp.open("w", encoding="utf-8") as fp:
            json.dump({"network": self.network, "devices": self.devices}, fp)
            fp.write("\n")
        tmp.chmod(0o600)
        tmp.replace(self.path)

    def _store_found(self, device):
        existing = self.devices.get(device["id"], {})
        for candidate in self.devices.values():
            if isinstance(candidate, dict) and candidate.get("physical_id") == device["id"]:
                existing = candidate
                break
        # Protect keys from being silently associated with a different board
        # or network address in legacy IP-based profiles.
        if existing.get("board_hash") and existing.get("board_hash") != device["board_hash"]:
            existing.pop("key", None)
        if existing.get("board_id") and existing.get("board_id") != device["board_id"]:
            existing.pop("key", None)
        new = dict(existing, **device)
        if existing.get("managed") and existing.get("name"):
            new["name"] = existing["name"]
        new["managed"] = bool(existing.get("managed", False))
        new["last_seen"] = datetime.now(timezone.utc).isoformat(timespec="seconds")
        self.devices[existing.get("id",device["id"])] = new
        new["id"] = existing.get("id",device["id"])
        if new["id"] != device["id"]:
            self.devices.pop(device["id"], None)

    def status(self):
        with self.lock:
            values = [{k: v for k, v in obj.items() if k not in ("key", "wifi_password", "board")} | 
                      {"has_key": bool(obj.get("key"))}
                      for obj in self.devices.values() if isinstance(obj, dict)]
            return {"devices": sorted(values, key=lambda x: x.get("name", "")),
                    "network": self.network, "scanning": self.scanning,
                    "refreshing": self.refreshing, "error": self.error}

    def set_network(self, network):
        value = scan_network(network)
        with self.lock:
            self.network = value
            self._persist()
        return self.status()

    def add(self, ip):
        result = probe(local_ip(ip), timeout=2)
        if not result:
            raise ValueError("No compatible native RFLink found at this address")
        with self.lock:
            self._store_found(result)
            self.devices[result["id"]]["managed"] = True
            self._persist()
        return self.status()

    @staticmethod
    def _name(value):
        if not isinstance(value, str) or not 1 <= len(value.strip()) <= 48:
            raise ValueError("Enter a device name (1-48 characters)")
        if any(ord(char) < 32 for char in value):
            raise ValueError("Device name contains an invalid control character")
        return value.strip()

    def adopt(self, device_id, name):
        name = self._name(name)
        with self.lock:
            if not isinstance(device_id, str) or device_id not in self.devices:
                raise ValueError("Select a discovered RFLink first")
            device = self.devices[device_id]
            if not device.get("online"):
                raise ValueError("RFLink must be online for first adoption")
            device["name"] = name
            device["managed"] = True
            self._persist()
        return self.status()

    def rename(self, device_id, name):
        name = self._name(name)
        with self.lock:
            if device_id not in self.devices or not self.devices[device_id].get("managed"):
                raise ValueError("Device is not managed")
            self.devices[device_id]["name"] = name
            self._persist()
        return self.status()

    def remove(self, device_id):
        with self.lock:
            if device_id not in self.devices:
                raise ValueError("RFLink not found")
            del self.devices[device_id]
            self._persist()
        return self.status()

    def create_config(self, name, board, board_hash, ssid, wifi_password, ota_key, firmware="current"):
        name = self._name(name)
        if not isinstance(ssid, str) or not 1 <= len(ssid.encode("utf-8")) <= 32:
            raise ValueError("Enter a Wi-Fi SSID (1-32 bytes)")
        if not isinstance(wifi_password, str) or not 8 <= len(wifi_password) <= 63:
            raise ValueError("Enter a Wi-Fi password of 8-63 characters")
        if not isinstance(ota_key, str) or not 20 <= len(ota_key) <= 64:
            raise ValueError("OTA password must be 20-64 characters")
        if any(ord(ch) < 32 for ch in (ssid + wifi_password + ota_key)):
            raise ValueError("Credentials contain control characters")
        if not isinstance(board, dict) or not isinstance(board_hash, str) or not re.fullmatch(r"[0-9a-f]{8}", board_hash):
            raise ValueError("Invalid board configuration")
        if firmware != "current" and not (
            isinstance(firmware,dict) and isinstance(firmware.get("tag"),str) and
            isinstance(firmware.get("sha"),str) and isinstance(firmware.get("release_id"),int) and
            isinstance(firmware.get("repo"),str)):
            raise ValueError("Firmware must be current source or a verified published release")
        with self.lock:
            ident = "cfg-" + secrets.token_hex(9)
            while ident in self.devices:
                ident = "cfg-" + secrets.token_hex(9)
            item = {"id": ident, "name": name, "ip": "", "online": False,
                    "managed": True, "configured": True, "provisioned": False,
                    "firmware": "", "firmware_choice": firmware,
                    "board_id": board["board_id"], "board_hash": board_hash,
                    "board": board, "wifi_ssid": ssid, "wifi_password": wifi_password,
                    "key": ota_key, "config_revision": secrets.token_hex(8), "created_at": datetime.now(timezone.utc).isoformat(timespec="seconds")}
            self.devices[ident] = item
            self._persist()
            return {"device_id": ident, "name": name, "has_key": True, "board_hash": board_hash}

    def get_config(self, device_id):
        with self.lock:
            item = self.devices.get(device_id)
            if not isinstance(item,dict) or not item.get("configured"):
                raise ValueError("Select a configured RFLink device")
            return dict(item)

    def config_public(self, device_id):
        item = self.get_config(device_id)
        return {"device_id": device_id, "name": item["name"], "board": item["board"],
                "board_hash": item["board_hash"], "wifi_ssid": item["wifi_ssid"],
                "wifi_password_saved": bool(item.get("wifi_password")),
                "ota_password_saved": bool(item.get("key")),
                "firmware_choice": item.get("firmware_choice","current")}

    def update_config(self, device_id, board, board_hash, ssid, wifi_password):
        with self.lock:
            item = self.devices.get(device_id)
            if not isinstance(item,dict) or not item.get("configured"):
                raise ValueError("Select a configured RFLink device")
            if item.get("provisioned") or item.get("physical_id"):
                raise ValueError("Hardware settings cannot be changed for an already provisioned device")
            if not isinstance(ssid,str) or not 1 <= len(ssid.encode("utf-8")) <= 32:
                raise ValueError("Invalid Wi-Fi SSID")
            if not wifi_password:
                if ssid != item["wifi_ssid"]:
                    raise ValueError("Enter a new Wi-Fi password when changing networks")
                wifi_password = item["wifi_password"]
            if not isinstance(wifi_password,str) or not 8 <= len(wifi_password) <= 63:
                raise ValueError("Wi-Fi password must be 8-63 characters")
            item.update(board=board,board_hash=board_hash,board_id=board["board_id"],
                        wifi_ssid=ssid,wifi_password=wifi_password,config_revision=secrets.token_hex(8))
            self._persist()
            return self.config_public(device_id)

    def link(self, config_id, discovered_id):
        # A user must explicitly confirm the device, never auto-match two
        # boards solely because their GPIO profile hashes are identical.
        with self.lock:
            configured = self.devices.get(config_id)
            target = self.devices.get(discovered_id)
            if not isinstance(configured,dict) or not configured.get("configured"):
                raise ValueError("Choose a previously configured RFLink")
            if not isinstance(target,dict) or target.get("managed") or not target.get("online"):
                raise ValueError("Choose an online, unclaimed RFLink")
            if configured.get("board_id")!=target.get("board_id") or configured.get("board_hash")!=target.get("board_hash"):
                raise ValueError("Discovered RFLink hardware profile does not match this configuration")
            configured.update(ip=target["ip"],online=True,physical_id=discovered_id,
                              firmware=target["firmware"],provisioned=True,
                              identity_verified=target.get("identity_verified",False),
                              last_seen=target.get("last_seen",""))
            self.devices.pop(discovered_id,None)
            self._persist()
        return self.status()

    def generate_key(self):
        # 24 bytes from the OS CSPRNG; 192 bits of entropy.
        return {"key": secrets.token_urlsafe(24)}

    def mark_provisioned(self, device_id):
        with self.lock:
            if device_id in self.devices and self.devices[device_id].get("configured"):
                self.devices[device_id]["provisioned"] = True
                self._persist()

    def discover(self):
        with self.lock:
            if self.scanning:
                return self.status()
            self.scanning = True
            self.error = ""
        threading.Thread(target=self._scan, daemon=True, name="rflink-discovery").start()
        return self.status()

    def _scan(self):
        with self.lock:
            # Known local network, or two common domestic subnet defaults.
            # Set your own /24 once for other networks or VLANs.
            networks = [self.network] if self.network else ["192.168.178.0/24", "192.168.1.0/24"]
            ips = {ip for d in self.devices.values() if isinstance(d, dict)
                   for ip in [d.get("ip", "")] if self._safe_ip(ip)}
            for n in networks:
                ips.update(str(ip) for ip in ipaddress.IPv4Network(n).hosts())
            for value in self.devices.values():
                if isinstance(value, dict): value["online"] = False
        try:
            with ThreadPoolExecutor(max_workers=DISCOVERY_WORKERS) as pool:
                tasks = {pool.submit(probe, ip): ip for ip in sorted(ips)}
                for future in as_completed(tasks):
                    try:
                        result = future.result()
                    except (OSError, ValueError, json.JSONDecodeError):
                        continue
                    if result:
                        with self.lock:
                            self._store_found(result)
            with self.lock:
                self._persist()
        except Exception:
            with self.lock:
                self.error = "Discovery failed; try a smaller LAN range or add an IP manually"
        finally:
            with self.lock:
                self.scanning = False

    def refresh_known(self):
        """Probe existing device IPs, without scanning entire subnets."""
        with self.lock:
            if self.scanning or self.refreshing:
                return
            self.refreshing = True
        threading.Thread(target=self._refresh, daemon=True, name="rflink-device-health").start()

    def _refresh(self):
        try:
            with self.lock:
                known = [(device_id, device.get("ip", ""))
                         for device_id, device in self.devices.items()
                         if isinstance(device, dict) and self._safe_ip(device.get("ip", ""))]
            with ThreadPoolExecutor(max_workers=HEALTH_WORKERS) as pool:
                checks = {pool.submit(probe, ip, timeout=1.0): (device_id, ip)
                          for device_id, ip in known}
                for task in as_completed(checks):
                    device_id, ip = checks[task]
                    try:
                        result = task.result()
                    except (OSError, ValueError, json.JSONDecodeError):
                        result = None
                    with self.lock:
                        old = self.devices.get(device_id)
                        if not isinstance(old, dict) or old.get("ip") != ip:
                            continue
                        old["online"] = False
                        if result and result["id"] == old.get("physical_id", device_id) and result["board_id"] == old.get("board_id"):
                            self._store_found(result)
                
            with self.lock:
                self._persist()
        finally:
            with self.lock:
                self.refreshing = False

    @staticmethod
    def _safe_ip(ip):
        try:
            local_ip(ip)
            return True
        except (ValueError, TypeError):
            return False

    def save_key(self, device_id, key):
        if not isinstance(key, str) or not 12 <= len(key) <= 64:
            raise ValueError("OTA key must be 12 to 64 characters")
        with self.lock:
            if device_id not in self.devices or not self.devices[device_id].get("managed"):
                raise ValueError("Adopt a RFLink before saving its password")
            if self.devices[device_id].get("configured") and self.devices[device_id].get("provisioned"):
                raise ValueError("Cannot change OTA password after first flash without changing it on the RFLink")
            self.devices[device_id]["key"] = key
            if self.devices[device_id].get("configured"):
                self.devices[device_id]["config_revision"] = secrets.token_hex(8)
            self._persist()
        return self.status()

    def get_key(self, device_id):
        with self.lock:
            obj = self.devices.get(device_id)
            if not isinstance(obj, dict) or not obj.get("key"):
                raise ValueError("No OTA password saved for the selected RFLink")
            return obj["key"]

    def get_wifi_password(self, device_id):
        return self.get_config(device_id).get("wifi_password", "")

    def is_configured(self, device_id):
        with self.lock:
            return bool(self.devices.get(device_id,{}).get("configured"))

    def delete_key(self, device_id):
        with self.lock:
            if device_id not in self.devices:
                raise ValueError("RFLink not found")
            self.devices[device_id].pop("key", None)
            self._persist()
        return self.status()

    def get_target(self, device_id):
        with self.lock:
            item = self.devices.get(device_id)
            if not isinstance(item, dict):
                raise ValueError("Select a recognized RFLink")
            ip, key = item.get("ip", ""), item.get("key", "")
            if not key:
                raise ValueError("Save this RFLink's OTA password in Settings")
            return local_ip(ip), key
