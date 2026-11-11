"""Best-effort Python heap release after resource-intensive Builder activity.

Only run after a quiet period. This never deletes firmware, ESP-IDF tools,
source files, credentials, or live Python objects. glibc malloc_trim releases
unused allocator pages where possible; it cannot reclaim referenced objects,
Linux filesystem cache, or guarantee a smaller process RSS.
"""
from __future__ import annotations

import gc
import threading
import time
from datetime import datetime, timezone

from memory_diagnostics import _process_details


def _trim_allocator() -> tuple[bool, bool]:
    """Return (glibc trim available, glibc reported pages returned)."""
    try:
        import ctypes
        libc = ctypes.CDLL(None)
        trim = getattr(libc, "malloc_trim", None)
        if trim is None:
            return False, False
        trim.argtypes = [ctypes.c_size_t]
        trim.restype = ctypes.c_int
        return True, bool(trim(0))
    except (OSError, AttributeError, TypeError):
        # Other libc implementations may not provide malloc_trim.
        return False, False


class IdleMemoryReclaimer:
    GRACE_SECONDS = 10
    REASONS = frozenset(("build", "building", "scanning",
                         "installing", "toolchain_setup"))

    def __init__(self):
        self._lock = threading.Lock()
        self._requested_at = None
        self._reason = None
        self._last = None

    def request(self, reason: str, now=None) -> None:
        if reason not in self.REASONS:
            raise ValueError("Unknown idle memory release reason")
        when = time.monotonic() if now is None else now
        with self._lock:
            self._requested_at = when
            self._reason = reason

    def run_if_idle(self, idle: bool, now=None):
        """Collect cycles and return idle allocator pages once per request.

        The existing five-second monitor invokes this without adding a thread.
        Requests are retained during active builds, scans and installations.
        """
        if not idle:
            return None
        when = time.monotonic() if now is None else now
        with self._lock:
            if (self._requested_at is None or
                    when - self._requested_at < self.GRACE_SECONDS):
                return None
            reason = self._reason
            self._reason = None
            self._requested_at = None
        before = _process_details()
        collected = gc.collect()
        supported, returned = _trim_allocator()
        after = _process_details()
        record = {
            "time": datetime.now(timezone.utc).isoformat(timespec="seconds"),
            "reason": reason,
            "gc_cycles_collected": collected,
            "allocator_trim_supported": supported,
            "allocator_returned_pages": returned,
            "python_rss_before_bytes": before.get("python_rss_bytes"),
            "python_rss_after_bytes": after.get("python_rss_bytes"),
            "python_anon_before_bytes": before.get("python_anon_bytes"),
            "python_anon_after_bytes": after.get("python_anon_bytes"),
        }
        with self._lock:
            self._last = record
        return dict(record)

    def snapshot(self):
        with self._lock:
            return {"pending": self._requested_at is not None,
                    "grace_seconds": self.GRACE_SECONDS,
                    "last": dict(self._last) if self._last else None}
