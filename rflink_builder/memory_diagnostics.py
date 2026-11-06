"""Lightweight, read-only memory diagnostics for Home Assistant RFLink Builder.

Uses Linux cgroup counters for the *whole app container*, plus the Python
server's resident set size. No psutil, shell commands or subprocesses.
Linux cgroup memory.current includes filesystem cache and compiler children;
Python RSS alone must not be reported as total add-on memory.
"""
from __future__ import annotations

from collections import deque
from datetime import datetime, timezone
from pathlib import Path
import threading
import time


def _number(path: Path):
    try:
        text = path.read_text(encoding="ascii").strip()
        return int(text) if text.isdecimal() else None
    except (OSError, ValueError):
        return None


def _process_details():
    """Cheap process counters. Anonymous RSS vs file mappings and thread count
    help distinguish webserver allocations from many idle HTTP connections.
    Do not inspect process environment, command lines or any secrets.
    """
    fields = {"VmRSS": "python_rss_bytes",
              "RssAnon": "python_anon_bytes",
              "RssFile": "python_file_bytes",
              "VmHWM": "python_peak_rss_bytes",
              "VmSwap": "python_swap_bytes"}
    result = {name: None for name in fields.values()}
    result["python_threads"] = None
    result["python_fds"] = None
    try:
        for line in Path("/proc/self/status").read_text(encoding="ascii").splitlines():
            key, sep, value = line.partition(":")
            if not sep:
                continue
            number = value.strip().split()
            if not number or not number[0].isdecimal():
                continue
            if key in fields:
                result[fields[key]] = int(number[0]) * 1024  # /proc/status reports KiB.
            elif key == "Threads":
                result["python_threads"] = int(number[0])
    except OSError:
        pass
    try:
        # Just a descriptor count; never read links/paths or open descriptors.
        result["python_fds"] = len(list(Path("/proc/self/fd").iterdir()))
    except OSError:
        pass
    return result


def _host_ram():
    try:
        for line in Path("/proc/meminfo").read_text(encoding="ascii").splitlines():
            if line.startswith("MemTotal:"):
                return int(line.split()[1]) * 1024
    except (OSError, ValueError, IndexError):
        pass
    return None


def _cache_bytes(path: Path):
    try:
        fields = {}
        for line in path.read_text(encoding="ascii").splitlines():
            key, _, value = line.partition(" ")
            if value.isdecimal():
                fields[key] = int(value)
        # v2 has 'file'; v1 has 'total_cache' and 'cache'.
        return fields.get("file", fields.get("total_cache", fields.get("cache")))
    except OSError:
        return None


def read_memory():
    """Return bytes and properly-labelled denominators; never guess HA UI %."""
    root = Path("/sys/fs/cgroup")
    if (root / "memory.current").is_file():
        used = _number(root / "memory.current")
        limit = _number(root / "memory.max")
        kernel_peak = _number(root / "memory.peak")
        cache = _cache_bytes(root / "memory.stat")
        source = "cgroup_v2"
    elif (root / "memory/memory.usage_in_bytes").is_file():
        root = root / "memory"
        used = _number(root / "memory.usage_in_bytes")
        limit = _number(root / "memory.limit_in_bytes")
        kernel_peak = _number(root / "memory.max_usage_in_bytes")
        cache = _cache_bytes(root / "memory.stat")
        source = "cgroup_v1"
    else:
        used = limit = kernel_peak = cache = None
        source = "process_only"

    host = _host_ram()
    # Some cgroup v1 installations use a huge sentinel for "unlimited".
    if limit is not None and (limit <= 0 or (host is not None and limit > host * 4)):
        limit = None
    return {
        "source": source,
        "container_bytes": used,
        "container_peak_bytes": kernel_peak,
        "container_limit_bytes": limit,
        **_process_details(),
        "cache_bytes": cache,
        "host_total_bytes": host,
        "percent_of_host": round(100 * used / host, 1) if used is not None and host else None,
        "percent_of_limit": round(100 * used / limit, 1) if used is not None and limit else None,
    }


class MemoryMonitor:
    """Sample every five seconds. Keep only an in-RAM window, no secrets or logs."""

    POST_BUILD_SECONDS = 15

    def __init__(self):
        self.lock = threading.RLock()
        self.current = None
        self.last_idle = None
        self.observed_peak_bytes = None
        self.last_build = None
        self.history = deque(maxlen=120)
        self.startup_checkpoints = deque(maxlen=24)

    @staticmethod
    def _point(reading, phase):
        return {
            "phase": phase,
            "time": datetime.now(timezone.utc).isoformat(timespec="seconds"),
            **reading,
        }

    def sample(self, phase: str, reading=None, now=None):
        reading = read_memory() if reading is None else dict(reading)
        now = time.monotonic() if now is None else now
        point = self._point(reading, phase)
        amount = reading.get("container_bytes")
        with self.lock:
            self.current = point
            self.history.append({
                "time": point["time"], "phase": phase,
                "container_bytes": amount,
                "python_rss_bytes": reading.get("python_rss_bytes"),
            })
            if amount is not None:
                if self.observed_peak_bytes is None or amount > self.observed_peak_bytes:
                    self.observed_peak_bytes = amount
            if phase == "idle":
                self.last_idle = point
            build = self.last_build
            if build:
                if phase == "building" and not build.get("finished"):
                    if amount is not None and (build["peak_bytes"] is None or amount > build["peak_bytes"]):
                        build["peak_bytes"] = amount
                if build.get("finished") and build.get("after") is None:
                    if phase == "idle" and now - build["ended_at"] >= self.POST_BUILD_SECONDS:
                        build["after"] = point
        return point

    def checkpoint(self, label: str, reading=None):
        """Record a *named* startup boundary without storing device secrets."""
        reading = read_memory() if reading is None else dict(reading)
        point = {
            "stage": label,
            "time": datetime.now(timezone.utc).isoformat(timespec="seconds"),
            "container_bytes": reading.get("container_bytes"),
            "python_rss_bytes": reading.get("python_rss_bytes"),
            "python_anon_bytes": reading.get("python_anon_bytes"),
            "python_threads": reading.get("python_threads"),
            "python_fds": reading.get("python_fds"),
            "cache_bytes": reading.get("cache_bytes"),
        }
        with self.lock:
            self.startup_checkpoints.append(point)
        return dict(point)

    def start_build(self, job_id: str):
        # Called immediately before Builder changes state to running.
        point = self._point(read_memory(), "before_build")
        with self.lock:
            self.last_build = {
                "job_id": job_id,
                "before": point,
                "peak_bytes": point["container_bytes"],
                "after": None,
                "finished": False,
                "ended_at": None,
            }

    def finish_build(self, job_id: str):
        point = self._point(read_memory(), "build_finished")
        with self.lock:
            build = self.last_build
            if build and build["job_id"] == job_id and not build["finished"]:
                amount = point.get("container_bytes")
                if amount is not None and (build["peak_bytes"] is None or amount > build["peak_bytes"]):
                    build["peak_bytes"] = amount
                build["finished"] = True
                build["ended_at"] = time.monotonic()

    def snapshot(self):
        with self.lock:
            build = dict(self.last_build) if self.last_build else None
            if build:
                build.pop("ended_at", None)
            return {
                "current": dict(self.current) if self.current else None,
                "last_idle": dict(self.last_idle) if self.last_idle else None,
                "observed_peak_bytes": self.observed_peak_bytes,
                "last_build": build,
                "history": list(self.history),
                "interval_seconds": 5,
                "startup_checkpoints": list(self.startup_checkpoints),
                "notes": [
                    "Container usage includes compiler processes and Linux filesystem cache.",
                    "Python RSS measures only the Builder webserver, not its child compiler processes.",
                    "Percentages use explicit host RAM or cgroup limits and may differ from Home Assistant's resource card.",
                    "Sampling is every 5 seconds: very short spikes can be missed. Cgroup peak is since container start when supported.",
                ],
            }
