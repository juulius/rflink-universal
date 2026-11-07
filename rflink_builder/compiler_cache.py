"""Bounded, persistent on-disk ccache for disposable official ESP-IDF builds.

The compiler cache is separate from build_workspace and has no RFLink NVS
secrets. A cache problem never changes the actual firmware compiler or skips
release validation: we simply compile normally without ccache.
"""
from __future__ import annotations

from pathlib import Path
import shutil
import subprocess

CACHE_SIZE_MIB=384
CACHE_SIZE=f"{CACHE_SIZE_MIB}M"


def setup_cache(data_root: Path, env: dict) -> tuple[dict, dict]:
    """Return a copied IDF environment and an explicit diagnostic snapshot."""
    settings=dict(env)
    diag={"enabled":False,"limit_mib":CACHE_SIZE_MIB,
          "path":"","message":"Compiler cache unavailable; compiling normally."}
    settings["IDF_CCACHE_ENABLE"]="0"  # Never inherit an unverified external cache.
    if settings.get("RFLINK_DISABLE_CCACHE")=="1":
        diag["message"]="Compiler cache disabled for diagnostics."
        return settings,diag
    if not shutil.which("ccache",path=settings.get("PATH")):
        diag["message"]="ccache executable is unavailable. Compiling normally."
        return settings,diag
    root=Path(data_root).resolve()
    location=root/"compiler_cache"
    try:
        # Never follow an attacker-controlled alias into an unrelated folder.
        if location.is_symlink():
            raise OSError("Cache directory is a symbolic link")
        location.mkdir(parents=True,exist_ok=True,mode=0o700)
        location.chmod(0o700)
        if not location.is_dir():
            raise OSError("Compiler cache is not a directory")
    except OSError:
        diag["message"]="Compiler cache folder unavailable. Compiling normally."
        return settings,diag
    settings.update({
        "IDF_CCACHE_ENABLE":"1",
        "CCACHE_DIR":str(location),
        "CCACHE_MAXSIZE":CACHE_SIZE,
        "CCACHE_COMPRESS":"1",
        "CCACHE_COMPRESSLEVEL":"5",
        "CCACHE_UMASK":"077",
    })
    diag.update(enabled=True,path=str(location),
                message=f"ccache enabled, max {CACHE_SIZE_MIB} MiB on disk (not reserved RAM).")
    return settings,diag


def cache_counters(env: dict) -> dict:
    """Read cumulative hit/miss counters, without resetting global stats."""
    if env.get("IDF_CCACHE_ENABLE")!="1":
        return {}
    try:
        # ccache 4.x uses stable, machine-readable tab-separated IDs with
        # --print-stats. The actual counter IDs are *direct_cache_hit*
        # and *preprocessed_cache_hit*, not the human UI labels.
        result=subprocess.run(
            ["ccache","--print-stats"],env=env,
            check=False,capture_output=True,text=True,timeout=8)
        if result.returncode or len(result.stdout)>128*1024:
            return {}
        values={}
        for line in result.stdout.splitlines():
            key,sep,value=line.partition("\t")
            if not sep:continue
            if key in ("direct_cache_hit","preprocessed_cache_hit","cache_miss"):
                values[key]=int(value)
        if any(type(v) is not int or v<0 for v in values.values()):
            return {}
        return values if len(values)==3 else {}
    except (OSError,ValueError,subprocess.TimeoutExpired):
        return {}


def cache_delta(previous:dict, current:dict) -> dict:
    """Hits from THIS build (not the lifetime hit rate)."""
    required=("direct_cache_hit","preprocessed_cache_hit","cache_miss")
    if not all(key in previous and key in current for key in required):
        return {}
    hits=sum(max(0,current[key]-previous[key]) for key in required[:2])
    misses=max(0,current["cache_miss"]-previous["cache_miss"])
    total=hits+misses
    return {"hits":hits,"misses":misses,"requests":total,
            "hit_rate_percent":round(100*hits/total,1) if total else None}
