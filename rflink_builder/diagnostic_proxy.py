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
STREAMS={"all":"all","signal":"signal","noise":"noise",
         "system":"system","web":"web"}

class NoRedirect(HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        return None


def diagnostics_for_device(device_book, device_id, stream="all"):
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
    def fetch(endpoint, limit):
        req=Request("http://"+str(addr)+endpoint,headers={"Accept":"application/json"})
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
    return {"device_id":device_id,"ip":str(addr),
            "firmware":str(status.get("firmware",""))[:96],
            "wifi_connected":status.get("wifi_connected") is True,
            "events":safe}
