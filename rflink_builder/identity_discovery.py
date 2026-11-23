"""Conservative RFLink DNS candidate resolution.

DNS only supplies possible *addresses*. It never authenticates a device:
callers must probe /api/status and verify the physical MAC, board and hash
before transferring any saved Builder association or OTA credentials.
"""
from __future__ import annotations
import ipaddress
import re
import socket

IDENTITY = re.compile(r"rf-[0-9a-f]{12}\Z")
LABEL = re.compile(r"[a-z0-9](?:[a-z0-9-]{0,61}[a-z0-9])?\Z")
MAX_DNS_CANDIDATES = 4


def dns_domain(value):
    if not isinstance(value, str):
        raise ValueError("DNS suffix must be a domain, e.g. home.arpa")
    domain = value.strip().rstrip(".").lower()
    if not domain:
        return ""
    if len(domain) > 190 or any(not LABEL.fullmatch(label)
                                for label in domain.split(".")):
        raise ValueError("Enter a valid DNS suffix, e.g. home.arpa")
    if domain.endswith(".local") or domain == "local":
        raise ValueError("Use mDNS for .local names, not ordinary DNS")
    return domain


def physical_hostname(ident):
    if not isinstance(ident, str) or not IDENTITY.fullmatch(ident):
        return ""
    return "rflink-" + ident[3:]


def safe_dns_ip(value):
    try:
        address = ipaddress.IPv4Address(value)
    except (ValueError, TypeError):
        return None
    # Guard HTTP candidate probes from public, metadata/link-local,
    # loopback and other reserved ranges, even if a resolver is poisoned.
    if not address.is_private or not any(address in subnet for subnet in (
        ipaddress.IPv4Network("10.0.0.0/8"),
        ipaddress.IPv4Network("172.16.0.0/12"),
        ipaddress.IPv4Network("192.168.0.0/16"),
    )):
        return None
    return str(address)


def dns_candidates(ident, domain, *, resolver=None):
    name = physical_hostname(ident)
    if not name:
        return []
    domain = dns_domain(domain)
    hostname = name + ("." + domain if domain else "")
    resolver = resolver or socket.getaddrinfo
    try:
        answers = resolver(hostname, 80, socket.AF_INET, socket.SOCK_STREAM)
    except (socket.gaierror, OSError, TimeoutError, ValueError):
        return []
    allowed = []
    for entry in answers[:32]:
        try:
            address = safe_dns_ip(entry[4][0])
        except (TypeError, KeyError, IndexError):
            continue
        if address and address not in allowed:
            allowed.append(address)
        if len(allowed) >= MAX_DNS_CANDIDATES:
            break
    return allowed


def find_verified_dns(ident, board, board_hash, domain, probe):
    """Return one exact identity+profile match or None; reject ambiguity."""
    matches = {}
    for ip in dns_candidates(ident, domain):
        try:
            found = probe(ip, timeout=1.5)
        except (OSError, ValueError):
            continue
        if (isinstance(found, dict) and found.get("ip") == ip and
                found.get("id") == ident and
                found.get("identity_verified") is True and
                found.get("board_id") == board and
                found.get("board_hash") == board_hash):
            matches[ip] = found
    return next(iter(matches.values())) if len(matches) == 1 else None
