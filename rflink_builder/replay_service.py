"""Read-only, bounded RFLink replay service for authenticated Home Assistant Ingress.

RF exports are processed only in a temporary directory and never forwarded to
the ESP32, the internet, or a durable Builder database. All native codec code
is compiled once from the pinned firmware ZIP during Builder image creation.
"""
from __future__ import annotations

import hashlib
import os
import tempfile
import threading
from pathlib import Path

from replay_tools import rf_replay

MAX_REPLAY_BYTES = 16 * 1024 * 1024
MAX_REPLAY_RESULTS = 5000
_REPLAY_SLOT = threading.BoundedSemaphore(1)


class ReplayBusy(RuntimeError):
    """A user already has a replay in progress."""


def analyze_bytes(upload: bytes, kind: str) -> dict:
    if kind not in ("csv", "json"):
        raise rf_replay.ReplayError("upload must be CSV or RF-capture JSON")
    if not isinstance(upload, bytes) or not upload or len(upload) > MAX_REPLAY_BYTES:
        raise rf_replay.ReplayError("upload must be between 1 byte and 16 MiB")
    if not _REPLAY_SLOT.acquire(blocking=False):
        raise ReplayBusy("Another replay is running; please retry when it finishes.")
    try:
        with tempfile.TemporaryDirectory(prefix="rflink-offline-replay-") as folder:
            path = Path(folder) / ("captures.csv" if kind == "csv" else "capture.json")
            path.write_bytes(upload)
            captures, counters, problems = rf_replay.load_captures(path)
            total = len(captures)
            limited = total > MAX_REPLAY_RESULTS
            if limited:
                captures = captures[:MAX_REPLAY_RESULTS]
            report = rf_replay.make_report(captures, counters, problems)
            report["limits"] = {
                "input_bytes": len(upload),
                "total_replayable_captures": total,
                "analyzed_captures": len(captures),
                "results_truncated": limited,
                "max_results": MAX_REPLAY_RESULTS,
                "raw_captures_persisted": False,
            }
            # A published binary fingerprint permits comparing old/new decoder
            # outcomes without exposing code paths, raw payloads or identities.
            executable = Path(os.environ.get("RFLINK_RF_REPLAY_CODECS_BIN", ""))
            report["decoder_fingerprint"] = (
                hashlib.sha256(executable.read_bytes()).hexdigest()[:16]
                if executable.is_file() else "unavailable"
            )
            return report
    finally:
        _REPLAY_SLOT.release()
