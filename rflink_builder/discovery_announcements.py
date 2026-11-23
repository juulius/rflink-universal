"""Best-effort RFLink DNS-SD discovery for the Home Assistant Builder.

An mDNS announcement is an UNTRUSTED address hint, never authorization to
bind OTA credentials. Every candidate is probed through the native HTTP
status API before entering the unpaired device list or updating a managed IP.
The optional HA multicast forwarding path may not reach another VLAN.
"""
from __future__ import annotations

from concurrent.futures import ThreadPoolExecutor
import threading
import time

from identity_discovery import physical_hostname, safe_dns_ip
from devices import probe

SERVICE="_rflink._tcp.local."
MDNS_WORKERS=2
MAX_QUEUED=12
RETRY_DELAY=45


class RFLinkAnnouncements:
    def __init__(self, device_book, clock=None):
        self.book=device_book
        self.clock=clock or time.monotonic
        self.lock=threading.Lock()
        self.pending=set()
        self.last_attempt={}
        self.pool=None
        self.zeroconf=None
        self.browser=None
        self.ip_version=None

    def start(self):
        try:
            from zeroconf import IPVersion, ServiceBrowser, Zeroconf
            self.ip_version=IPVersion
            self.zeroconf=Zeroconf(ip_version=IPVersion.V4Only)
            self.pool=ThreadPoolExecutor(
                max_workers=MDNS_WORKERS,thread_name_prefix="rflink-mdns")
            self.browser=ServiceBrowser(self.zeroconf,SERVICE,self)
        except (ImportError, OSError, ValueError, RuntimeError) as exc:
            self.book.discovery_mdns="unavailable"
            print("RFLink mDNS listening unavailable:",type(exc).__name__,flush=True)
            return self
        self.book.discovery_mdns="listening"
        return self

    def add_service(self, zeroconf, service_type, name):
        self._schedule(service_type,name)

    def update_service(self, zeroconf, service_type, name):
        self._schedule(service_type,name)

    def remove_service(self, zeroconf, service_type, name):
        # Deleting a DNS-SD service is not proof the physical RFLink is offline.
        pass

    def _schedule(self, service_type, name):
        if (service_type != SERVICE or not isinstance(name,str) or
                not name.endswith("."+SERVICE) or len(name)>180 or
                self.pool is None):
            return
        with self.lock:
            now=self.clock()
            if name in self.pending or len(self.pending)>=MAX_QUEUED:
                return
            if now-self.last_attempt.get(name,-RETRY_DELAY)>=RETRY_DELAY:
                self.last_attempt[name]=now
                self.pending.add(name)
            else:
                return
            if len(self.last_attempt)>128:
                self.last_attempt={
                    k:v for k,v in self.last_attempt.items()
                    if now-v < RETRY_DELAY}
        self.pool.submit(self._resolve_and_probe,service_type,name)

    def _resolve_and_probe(self,service_type,name):
        try:
            info=self.zeroconf.get_service_info(service_type,name,timeout=1700)
            if not info or info.port!=80:
                return
            properties=info.properties or {}
            physical=properties.get(b"id",b"")
            try:
                physical=physical.decode("ascii")
            except (UnicodeDecodeError, AttributeError):
                return
            hostname=physical_hostname(physical)
            # Ignore lookalike advertisements that do not use the stable
            # unique service instance name; this is not cryptographic auth.
            if not hostname or name.lower()!=hostname+"."+SERVICE:
                return
            for value in info.parsed_addresses(self.ip_version.V4Only)[:4]:
                ip=safe_dns_ip(value)
                if not ip:
                    continue
                try:
                    found=probe(ip,timeout=1.5)
                except (OSError,ValueError):
                    continue
                if (isinstance(found,dict) and found.get("ip")==ip and
                        found.get("id")==physical and
                        found.get("identity_verified") is True):
                    self.book.accept_announced_device(found)
        finally:
            with self.lock:
                self.pending.discard(name)
