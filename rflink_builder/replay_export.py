"""Stateless, bounded, private RF Replay report attachment transport."""
import hmac
import json

MAX_REPORT_BYTES = 16 * 1024 * 1024
MAX_EXPORT_BODY = MAX_REPORT_BYTES + 1024


class ExportError(ValueError):
    def __init__(self, status, message):
        super().__init__(message)
        self.status = status


def report_attachment(body: bytes, csrf: str) -> bytes:
    if not body or len(body) > MAX_EXPORT_BODY:
        raise ExportError(413, "Replay report exceeds export limit")
    # Compact JSON.stringify escapes literal CR/LF inside strings.
    fields = body.split(b"\r\n", 3)
    if len(fields) != 3 or fields[2] != b"" or not fields[0].startswith(b"csrf=") or not fields[1].startswith(b"report="):
        raise ExportError(400, "Invalid replay export form")
    if not hmac.compare_digest(fields[0][5:], csrf.encode("ascii")):
        raise ExportError(403, "Invalid session code; refresh the page")
    contents = fields[1][7:]
    if len(contents) > MAX_REPORT_BYTES:
        raise ExportError(413, "Replay report exceeds export limit")
    try:
        report = json.loads(contents.decode("utf-8"))
        if (not isinstance(report, dict) or report.get("schema") != "rflink-offline-replay-v1"
                or not isinstance(report.get("results"), list) or len(report["results"]) > 5000
                or not isinstance(report.get("summary"), dict)):
            raise ValueError("invalid replay schema")
    except (ValueError, RecursionError) as exc:
        raise ExportError(400, "Invalid replay report") from exc
    return contents + b"\n"
