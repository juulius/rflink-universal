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


MAX_LIVE_EXPORT_BYTES = 96 * 1024 * 1024


def _report_for_input(path: Path, byte_count: int, *, live=False) -> dict:
    """Run the same bounded offline decoders on uploads and live CSV snapshots."""
    captures, counters, problems = rf_replay.load_captures(path)
    total = len(captures)
    limited = total > MAX_REPLAY_RESULTS
    if limited:
        # A live snapshot should show the most recent available captures.
        # Historical file imports retain their previous first-N semantics.
        captures = captures[-MAX_REPLAY_RESULTS:] if live else captures[:MAX_REPLAY_RESULTS]
    report = rf_replay.make_report(captures, counters, problems, rtl433=True)
    report["limits"] = {
        "input_bytes": byte_count,
        "total_replayable_captures": total,
        "analyzed_captures": len(captures),
        "results_truncated": limited,
        "max_results": MAX_REPLAY_RESULTS,
        "raw_captures_persisted": False,
        "source": "live_device" if live else "file_upload",
    }
    # An immutable decoder fingerprint makes A/B comparisons reproducible,
    # without storing RF captures on disk beyond this request.
    executable = Path(os.environ.get("RFLINK_RF_REPLAY_CODECS_BIN", ""))
    report["decoder_fingerprint"] = (
        hashlib.sha256(executable.read_bytes()).hexdigest()[:16]
        if executable.is_file() else "unavailable"
    )
    return report


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
            return _report_for_input(path, len(upload))
    finally:
        _REPLAY_SLOT.release()


def analyze_device_stream(open_source) -> dict:
    """Fetch one verified RFLink full CSV once; analyze without browser upload.

    open_source is a trusted zero-argument context manager factory from the
    existing identity-bound Builder device proxy, never a caller-supplied URL.
    The source stream is processed in 32-KiB blocks to an auto-deleted temp
    file. No RF packets, credentials, or CSV are persisted in Builder /data.
    """
    if not callable(open_source):
        raise rf_replay.ReplayError("No authenticated RFLink stream available")
    if not _REPLAY_SLOT.acquire(blocking=False):
        raise ReplayBusy("Another replay is running; please retry when it finishes.")
    try:
        with tempfile.TemporaryDirectory(prefix="rflink-live-replay-") as folder:
            path = Path(folder) / "captures.csv"
            received = 0
            with open_source() as source, path.open("wb") as destination:
                while True:
                    chunk = source.read(32 * 1024)
                    if not chunk:
                        break
                    if not isinstance(chunk, bytes):
                        raise rf_replay.ReplayError("Invalid device CSV response")
                    received += len(chunk)
                    if received > MAX_LIVE_EXPORT_BYTES:
                        raise rf_replay.ReplayError("RFLink export exceeds 96 MiB limit")
                    destination.write(chunk)
            if not received:
                raise rf_replay.ReplayError("RFLink returned an empty diagnostics export")
            report = _report_for_input(path, received, live=True)
            # A snapshot is not a live RF stream: explicitly state its limits.
            report["notes"].append(
                "This is one retained device snapshot, not continuous reception. "
                "Expired RF details cannot be reconstructed."
            )
            return report
    finally:
        _REPLAY_SLOT.release()
