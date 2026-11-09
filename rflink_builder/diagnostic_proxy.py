"""Read-only, device-bound RFLink diagnostic log proxy.

The browser talks to the authenticated Home Assistant ingress origin, not
directly to a LAN IP (mixed content/CORS). Never send OTA credentials or
follow redirects, and never permit arbitrary URLs/IPs or raw serial writes.
"""
from __future__ import annotations
import ipaddress
import json
import re
from urllib.request import Request, build_opener, ProxyHandler, HTTPRedirectHandler

MAX_LOG_RESPONSE=48*1024
MAX_CRASH_RESPONSE=12*1024
MAX_CONSOLE_RESPONSE=24*1024
STREAMS={"all":"all","signal":"signal","noise":"noise",
         "system":"system","web":"web"}


def _safe_crash_text(value, limit=80):
    """Only accept bounded public metadata; never proxy task stacks or secrets."""
    if not isinstance(value, str):
        return ""
    return re.sub(r"[^a-zA-Z0-9 _.:+-]", "", value[:limit])


def _safe_status_integer(source, key, minimum=0, maximum=65535):
    """Explicitly reject booleans, malformed or out-of-range radio diagnostics."""
    value = source.get(key)
    return value if type(value) is int and minimum <= value <= maximum else None


def _safe_crash_report(source):
    history = source.get("history")
    running = source.get("running_firmware")
    dump = source.get("core_dump")
    if not isinstance(history, list):
        history = []
    if not isinstance(running, dict):
        running = {}
    if not isinstance(dump, dict):
        dump = {}
    safe_history = []
    for entry in history[:12]:
        if not isinstance(entry, dict):
            continue
        safe_history.append({
            "boot": entry["boot"] if type(entry.get("boot")) is int
                and 0 <= entry["boot"] <= 4294967295 else 0,
            "reason": _safe_crash_text(entry.get("reason"), 48),
            "reset_code": entry["reset_code"] if type(entry.get("reset_code")) is int
                and 0 <= entry["reset_code"] <= 65535 else 0,
            "has_core_dump": entry.get("has_core_dump") is True,
            "pc": _safe_crash_text(entry.get("pc"), 12),
            "task": _safe_crash_text(entry.get("task"), 16),
        })
    phases = source.get("boot_phases")
    if not isinstance(phases, list):
        phases = []
    allowed_stages = {
        "nvs_ready", "probing_radios", "starting_receivers",
        "starting_network", "verifying_ota", "app_ready",
    }
    safe_phases = []
    for phase in phases[:12]:
        if not isinstance(phase, dict):
            continue
        boot = phase.get("boot")
        stage = phase.get("stage")
        if type(boot) is not int or not (0 < boot <= 4294967295):
            continue
        if not isinstance(stage, str) or stage not in allowed_stages:
            continue
        safe_phases.append({
            "boot": boot, "stage": stage,
            "startup_complete": phase.get("startup_complete") is True,
        })
    return {
        "phase_history_saved": source.get("phase_history_saved") is True,
        "boot_phases": safe_phases,
        "last_reset": _safe_crash_text(source.get("last_reset"), 48),
        "history_saved": source.get("history_saved") is True,
        "running_firmware": {
            "version": _safe_crash_text(running.get("version"), 32),
            "elf_sha256": _safe_crash_text(running.get("elf_sha256"), 64),
        },
        "core_dump": {
            "available": dump.get("available") is True,
            "summary_available": dump.get("summary_available") is True,
            "integrity": _safe_crash_text(dump.get("integrity"), 24),
            "build_comparison": _safe_crash_text(dump.get("build_comparison"), 32),
            "elf_sha256": _safe_crash_text(dump.get("elf_sha256"), 64),
            "pc": _safe_crash_text(dump.get("pc"), 12),
            "task": _safe_crash_text(dump.get("task"), 16),
        },
        "history": safe_history,
    }


class NoRedirect(HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        return None


def diagnostics_for_device(device_book, device_id, stream="all", *, include_crashes=False,
                           include_console=False, console_since=0):
    if not isinstance(device_id,str) or not re.fullmatch(r"(?:cfg-[0-9a-f]{18}|rf-[0-9a-f]{12}|ip:(?:[0-9]{1,3}\.){3}[0-9]{1,3})",device_id):
        raise ValueError("Choose an RFLink already saved in Builder")
    if stream not in STREAMS:
        raise ValueError("Invalid log stream")
    with device_book.lock:
        saved=device_book.devices.get(device_id)
        if not isinstance(saved,dict) or saved.get("managed") is not True:
            raise ValueError("RFLink is not saved in Builder")
        device=dict(saved)
    ip=device.get("ip")
    try:
        addr=ipaddress.IPv4Address(ip)
    except (ValueError,TypeError) as exc:
        raise ValueError("RFLink has no saved LAN address") from exc
    allowed=(ipaddress.IPv4Network("10.0.0.0/8"),
             ipaddress.IPv4Network("172.16.0.0/12"),
             ipaddress.IPv4Network("192.168.0.0/16"))
    if not any(addr in n for n in allowed):
        raise ValueError("Only private RFLink LAN addresses can be queried")
    opener=build_opener(ProxyHandler({}),NoRedirect())
    def fetch(endpoint, limit, headers=None):
        request_headers={"Accept":"application/json"}
        if headers:
            request_headers.update(headers)
        req=Request("http://"+str(addr)+endpoint,headers=request_headers)
        with opener.open(req,timeout=4) as response:
            if response.status!=200:raise RuntimeError("RFLink returned HTTP "+str(response.status))
            data=response.read(limit+1)
        if len(data)>limit:raise RuntimeError("RFLink diagnostic response is too large")
        result=json.loads(data)
        if not isinstance(result,dict):raise ValueError("Invalid RFLink JSON")
        return result
    status=fetch("/api/status",8192)
    if status.get("board_id") != device.get("board_id") or status.get("board_hash") != device.get("board_hash"):
        raise RuntimeError("RFLink hardware identity at this IP has changed")
    physical=device.get("physical_id")
    if physical and isinstance(physical,str) and physical.startswith("rf-"):
        if status.get("device_id") != physical:
            raise RuntimeError("Different RFLink found at the saved IP; diagnosis blocked")
    elif device_id.startswith("rf-") and status.get("device_id") != device_id:
        raise RuntimeError("RFLink device ID changed; diagnosis blocked")
    if include_crashes:
        return _safe_crash_report(fetch("/api/diagnostics/crashes", MAX_CRASH_RESPONSE))
    if include_console:
        # Unlike sanitized event metadata, raw ESP-IDF application logs can
        # contain implementation details. Device authorization is mandatory.
        key=device.get("key")
        if not isinstance(key,str) or not key:
            raise RuntimeError("Saved RFLink management key required for detailed console")
        if type(console_since) is not int or not 0 <= console_since < 2**53:
            raise ValueError("Invalid console cursor")
        result=fetch("/api/diagnostics/console?since="+str(console_since),
                     MAX_CONSOLE_RESPONSE,
                     {"X-RFLink-Update-Key":key})
        value=result.get("text")
        cursor=result.get("cursor")
        if not isinstance(value,str) or len(value)>8192 or type(cursor) is not int or not 0 <= cursor < 2**53:
            raise ValueError("RFLink console response is invalid")
        boot_id=result.get("boot_id")
        if type(boot_id) is not int or not 0 <= boot_id <= 4294967295:
            raise ValueError("RFLink console boot identity is invalid")
        return {"device_id":device_id,"text":value,"cursor":cursor,
                "boot_id":boot_id,
                "truncated":result.get("truncated") is True}
    q="" if stream=="all" else "?stream="+stream
    events=fetch("/api/logs"+q,MAX_LOG_RESPONSE).get("events")
    if not isinstance(events,list):raise ValueError("RFLink log response is invalid")
    safe=[]
    for event in events[:48]:
        if not isinstance(event,dict):continue
        safe.append({key:event[key] for key in (
            "sequence","uptime_ms","stream","radio","kind","severity","count",
            "value","correlation_id","tx_stage","tx_transport"
        ) if key in event and isinstance(event[key],(int,str))})
    # Values come from the running device's identity-verified /api/status,
    # never from the Builder's selected/previously compiled firmware release.
    # The lower-level RMT stage is distinct from ManagerError 7 (driver_failure).
    return {"device_id":device_id,"ip":str(addr),
            "firmware":_safe_crash_text(status.get("firmware"),96),
            "firmware_display":_safe_crash_text(status.get("firmware_display"),96),
            "release_channel":_safe_crash_text(status.get("release_channel"),32),
            "release_build":_safe_status_integer(status,"release_build",0,65535),
            "board_id":_safe_crash_text(status.get("board_id"),64),
            "board_hash":_safe_crash_text(status.get("board_hash"),32),
            "rx43342_listening":status.get("rx43342_listening") is True,
            "rx43392_listening":status.get("rx43392_listening") is True,
            "rx43342_rmt_error_stage":_safe_status_integer(status,"rx43342_rmt_error_stage",0,7),
            "rx43342_rmt_error_code":_safe_status_integer(status,"rx43342_rmt_error_code",-65535,65535),
            "rx43392_rmt_error_stage":_safe_status_integer(status,"rx43392_rmt_error_stage",0,7),
            "rx43392_rmt_error_code":_safe_status_integer(status,"rx43392_rmt_error_code",-65535,65535),
            "rx43342_start_error_stage":_safe_status_integer(status,"rx43342_start_error_stage",0,10),
            "rx43342_start_error_code":_safe_status_integer(status,"rx43342_start_error_code",-65535,65535),
            "rx43342_start_error_reg":_safe_status_integer(status,"rx43342_start_error_reg",-1,47),
            "rx43342_start_expected":_safe_status_integer(status,"rx43342_start_expected",-1,255),
            "rx43342_start_observed":_safe_status_integer(status,"rx43342_start_observed",-1,255),
            "rx43342_start_failures":_safe_status_integer(status,"rx43342_start_failures",0,4294967295),
            "rx43392_start_error_stage":_safe_status_integer(status,"rx43392_start_error_stage",0,10),
            "rx43392_start_error_code":_safe_status_integer(status,"rx43392_start_error_code",-65535,65535),
            "rx43392_start_error_reg":_safe_status_integer(status,"rx43392_start_error_reg",-1,47),
            "rx43392_start_expected":_safe_status_integer(status,"rx43392_start_expected",-1,255),
            "rx43392_start_observed":_safe_status_integer(status,"rx43392_start_observed",-1,255),
            "rx43392_start_failures":_safe_status_integer(status,"rx43392_start_failures",0,4294967295),
            "wifi_connected":status.get("wifi_connected") is True,
            "radio_initialized": status.get("radio_initialized")
                if type(status.get("radio_initialized")) is bool else None,
            "startup_complete": status.get("startup_complete")
                if type(status.get("startup_complete")) is bool else None,
            "events":safe}


def crash_report_for_device(device_book, device_id):
    """Sanitized, identity-checked reset history from a currently reachable ESP32."""
    return diagnostics_for_device(device_book, device_id, include_crashes=True)


def console_for_device(device_book, device_id, since=0):
    """Verified RFLink console, bound to the same saved physical identity as OTA."""
    return diagnostics_for_device(device_book,device_id,include_console=True,console_since=since)
