"""Bounded short-lived RF Replay attachments; never written to disk."""
import json
import secrets
import threading
import time

MAX_REPORT_BYTES = 16 * 1024 * 1024
TTL_SECONDS = 300
MAX_PENDING = 2


class ExportError(ValueError):
    def __init__(self, status, message):
        super().__init__(message)
        self.status = status


def report_attachment(body: bytes) -> bytes:
    if not body:
        raise ExportError(400, "Leeg replay-rapport")
    if len(body) > MAX_REPORT_BYTES:
        raise ExportError(413, "Replay-rapport groter dan 16 MiB")
    try:
        report = json.loads(body.decode("utf-8"))
        if (not isinstance(report, dict) or report.get("schema") != "rflink-offline-replay-v1"
                or not isinstance(report.get("results"), list) or len(report["results"]) > 5000
                or not isinstance(report.get("summary"), dict)):
            raise ValueError("invalid replay schema")
    except (ValueError, RecursionError) as exc:
        raise ExportError(400, "Ongeldig replay-rapport") from exc
    return body


class PendingReports:
    """Two bounded RAM-only reports, retryable GET until expiry."""
    def __init__(self):
        self._lock = threading.Lock()
        self._items = {}

    def _remove(self, token):
        item = self._items.pop(token, None)
        if item:
            item[2].cancel()

    def _expire(self, token):
        with self._lock:
            self._remove(token)

    def prepare(self, body):
        contents = report_attachment(body)
        token = secrets.token_urlsafe(32)
        with self._lock:
            while len(self._items) >= MAX_PENDING:
                self._remove(next(iter(self._items)))
            timer = threading.Timer(TTL_SECONDS, self._expire, args=(token,))
            timer.daemon = True
            self._items[token] = (contents, time.monotonic()+TTL_SECONDS, timer)
            timer.start()
        return token

    def get(self, token):
        with self._lock:
            item = self._items.get(token)
            if item is None:
                return None
            if time.monotonic() >= item[1]:
                self._remove(token)
                return None
            return item[0]

    def clear(self):
        with self._lock:
            for token in list(self._items):
                self._remove(token)


pending_reports = PendingReports()
