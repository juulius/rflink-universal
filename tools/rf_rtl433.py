"""Offline comparison with the separately packaged rtl_433 executable.

No hardware, configuration from uploads, shell, network output or inferred RF
edges. Only original OOK pulse pairs. See upstream docs/IQ_FORMATS.md.
"""
import collections
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import time

MAX_RUNS = 512
MAX_SECONDS = 25
MODEL = re.compile(r"[A-Za-z0-9][A-Za-z0-9_ .+()/:-]{0,79}")
MIC = {"CRC", "CHECKSUM", "PARITY", "MIC", "LFSR", "DIGEST"}


def ook_text(cap):
    if cap.get("radio") not in (0, 1) or cap.get("format") != "signed_pulses_us" or cap.get("capture_phase") != "pre_filter_rmt":
        return None
    p = cap["pulses_us"]
    if not 16 <= len(p) <= 1024 or any(type(v) is not int or not 0 < abs(v) <= 5000000 for v in p):
        return None
    if any((a>0)==(b>0) for a,b in zip(p,p[1:])):
        return None
    # File pairs must begin high and end low. Omit unmatched boundary
    # durations only; no fabricated sync gap, no rescaling or inversion.
    start = int(p[0]<0)
    end = len(p)-int(p[-1]>0)
    pairs = [(p[i],-p[i+1]) for i in range(start,end,2)]
    if len(pairs)<8:
        return None
    text = (";pulse data\n;version 1\n;timescale 1us\n"
            f";ook {len(pairs)} pulses\n;centerfreq {cap['frequency_hz']} Hz\n"
            ";samplerate 1000000 Hz\n" +
            "".join(f"{a} {b}\n" for a,b in pairs)+";end\n")
    return text, start+len(p)-end


def _safe_matches(stdout):
    # Never forward decoder-produced ID, channel, payload, measurements,
    # command, or untrusted arbitrary output. Local device grouping uses
    # anonymous report-scoped labels rather than stable identity hashes.
    matches = []
    for line in stdout.splitlines():
        if len(line)>16384:
            continue
        try:
            obj=json.loads(line)
        except ValueError:
            continue
        if not isinstance(obj,dict):
            continue
        model=obj.get("model")
        if not isinstance(model,str) or not MODEL.fullmatch(model):
            continue
        mic=obj.get("mic")
        mic=mic.upper() if isinstance(mic,str) else ""
        mic=mic if mic in MIC else "not_reported"
        protocol=obj.get("protocol")
        protocol=protocol if type(protocol) is int and 0<protocol<10000 else None
        ident=obj.get("id")
        channel=obj.get("channel")
        # Accept only primitive bounded identifier values for in-memory
        # grouping. A model without an ID never implies an individual device.
        def scalar(v):
            return v if (type(v) is int and abs(v)<2**64) or (isinstance(v,str) and 0<len(v)<=64) else None
        ident,channel=scalar(ident),scalar(channel)
        matches.append((model,mic,protocol,ident,channel))
        if len(matches)>=16:
            break
    return matches


def compare(captures, outputs, *, executable=None):
    exe=executable or shutil.which("rtl_433")
    stats=collections.Counter()
    report={"version":1,"state":"unavailable" if not exe else "complete",
            "summary":stats,"models":[],"device_candidates":[],
            "notes":["Independent decoder matches, not proven device ownership or reception rate.",
                     "Only original 433 OOK pulse pairs are compared. 868 bit windows are not OOK.",
                     "Integrity labels are reported by rtl_433, not independently recomputed.",
                     "Anonymous device labels are local to one report; raw identifiers are omitted."]}
    if not exe:
        return report
    # Fingerprint the installed executable for reproducible comparisons.
    # The executable comes from the trusted Builder package, never from input.
    try:
        with open(exe,"rb") as binary:
            report["executable_fingerprint"]=hashlib.file_digest(binary,"sha256").hexdigest()[:16]
    except OSError:
        report["executable_fingerprint"]=None
    started=time.monotonic()
    models={}
    devices={}
    seen=set()
    cache={}
    with tempfile.TemporaryDirectory(prefix="rflink-rtl433-") as tmp:
        path=Path(tmp)/"capture.ook"
        for cap,out in zip(captures,outputs):
            encoded=ook_text(cap)
            if encoded is None:
                continue
            cid=cap["capture_id"]
            if cid in seen:
                stats["duplicates_ignored"]+=1
                continue
            seen.add(cid)
            stats["eligible"]+=1
            data,omitted=encoded
            key=data
            if key not in cache:
                remaining=MAX_SECONDS-(time.monotonic()-started)
                if stats["runs"]>=MAX_RUNS or remaining<=0:
                    stats["not_tested_limit"]+=1
                    continue
                stats["runs"]+=1
                path.write_text(data,encoding="ascii")
                try:
                    proc=subprocess.run([exe,"-c",os.devnull,"-s","1000000",
                        "-f",str(cap["frequency_hz"]),"-M","protocol","-F","json",
                        "-r",str(path)],cwd=tmp,stdin=subprocess.DEVNULL,
                        stdout=subprocess.PIPE,stderr=subprocess.DEVNULL,
                        text=True,encoding="utf-8",errors="replace",
                        timeout=min(2,remaining),check=False)
                except (OSError,subprocess.TimeoutExpired):
                    stats["errors"]+=1
                    continue
                if proc.returncode:
                    stats["errors"]+=1
                    continue
                cache[key]=_safe_matches(proc.stdout)
            stats["tested"]+=1
            hits=cache[key]
            if not hits:
                continue
            stats["matched_captures"]+=1
            row=[]
            unique=set()
            for model,mic,protocol,ident,channel in hits:
                k=(cap["radio"],model,protocol)
                m=models.setdefault(k,{"radio":cap["radio"],"model":model,"protocol":protocol,
                    "captures":set(),"integrity":set()})
                m["captures"].add(cid)
                m["integrity"].add(mic)
                label=None
                if ident is not None:
                    dk=(k,ident,channel)
                    d=devices.setdefault(dk,{"label":f"D{len(devices)+1}",
                        "radio":cap["radio"],"model":model,"captures":set(),"integrity":set()})
                    d["captures"].add(cid)
                    d["integrity"].add(mic)
                    label=d["label"]
                rk=(model,mic,protocol,label)
                if rk not in unique:
                    row.append({"model":model,"integrity":mic,"protocol":protocol,"device_candidate":label})
                    unique.add(rk)
            out["rtl433"]={"matches":row,"omitted_boundary_durations":omitted,
                          "device_verified":False}
    def finish(items):
        rows=[{**g,"captures":len(g["captures"]),"integrity":sorted(g["integrity"])}
              for g in items]
        return sorted(rows,key=lambda x:(-x["captures"],x["model"]))[:40]
    report["models"]=finish(models.values())
    report["device_candidates"]=finish(devices.values())
    stats["model_groups"]=len(models)
    stats["device_candidates"]=len(devices)
    stats["repeated_device_candidates"]=sum(len(d["captures"])>=2 for d in devices.values())
    if stats["errors"] or stats["not_tested_limit"]:
        report["state"]="partial"
    report["summary"]=dict(stats)
    return report
