#!/usr/bin/env python3
"""Offline RFLink replay v1: 433 signed RMT pulses + 868 fixed FIFO packets.

No network, device connection, radio TX or firmware modifications. The optional
C++ helper is built only from this repository's existing decoder sources.
Reports omit packet bytes, RF device identities and signed pulse arrays unless
the user explicitly saves a *private* replay capture bundle.
"""
from __future__ import annotations

import argparse
import collections
import csv
import hashlib
import json
import math
import os
import re
import shutil
import statistics
import subprocess
import sys
import tempfile
from pathlib import Path

SCHEMA = "rflink-offline-replay-v1"
BUNDLE_SCHEMA = "rflink-replay-captures-v1"
MAX_CAPTURES = 20000
MAX_CSV_BYTES = 96 * 1024 * 1024
FREQ = {0: 433420000, 1: 433920000, 2: 868299900}
ROOT = Path(__file__).resolve().parents[1]
HEX_RE = re.compile(r"^(?:[0-9A-Fa-f]{2}(?:[ \t\r\n]*|$))+$")


class ReplayError(ValueError):
    pass


def _integer(value, name, minimum=None, maximum=None):
    if isinstance(value, bool):
        raise ReplayError(name + " must be an integer")
    try:
        out = int(value)
    except (ValueError, TypeError, OverflowError):
        raise ReplayError(name + " must be an integer") from None
    if str(value).strip() != str(out) and not isinstance(value, int):
        raise ReplayError(name + " is not a canonical integer")
    if minimum is not None and out < minimum:
        raise ReplayError(name + " is too small")
    if maximum is not None and out > maximum:
        raise ReplayError(name + " is too large")
    return out


def _parse_capture(obj, meta=None):
    if not isinstance(obj, dict):
        raise ReplayError("capture must be a JSON object")
    meta = meta or {}
    radio = _integer(obj.get("radio", meta.get("radio")), "radio", 0, 2)
    frequency = _integer(obj.get("frequency_hz", FREQ[radio]), "frequency_hz")
    if abs(frequency - FREQ[radio]) > 200000:
        raise ReplayError("frequency does not match radio")
    fmt = obj.get("format") or obj.get("detail_type")
    if fmt not in ("fifo_bytes_hex", "signed_pulses_us"):
        raise ReplayError("missing or unsupported capture format")
    if fmt == "fifo_bytes_hex" and radio != 2:
        raise ReplayError("FIFO format cannot be assigned to a 433 RMT radio")
    if fmt == "signed_pulses_us" and radio == 2:
        raise ReplayError("RMT pulse format cannot be assigned to the 868 FIFO radio")
    if fmt == "fifo_bytes_hex":
        raw = obj.get("hex", obj.get("fifo_bytes_hex"))
        if not isinstance(raw, str) or not raw or not HEX_RE.fullmatch(raw):
            raise ReplayError("invalid FIFO hex")
        data = bytes.fromhex(raw)
        if len(data) != 63:
            raise ReplayError("868 FIFO capture must contain exactly 63 bytes")
        normalized = {"hex": data.hex().upper()}
        actual = len(data)
    else:
        pulses = obj.get("pulses_us")
        if not isinstance(pulses, list) or not (1 <= len(pulses) <= 1024):
            raise ReplayError("RMT capture must have 1..1024 signed durations")
        normalized = {"pulses_us": [
            _integer(v, "signed pulse duration", -5000000, 5000000)
            for v in pulses]}
        if 0 in normalized["pulses_us"]:
            raise ReplayError("RMT pulse durations must be non-zero")
        actual = len(pulses)
    claimed = obj.get("count", actual)
    if _integer(claimed, "capture length") != actual:
        raise ReplayError("stored capture length does not match payload")
    capture = {
        "radio": radio, "frequency_hz": frequency, "format": fmt,
        "capture_phase": obj.get("capture_phase", meta.get("capture_phase", "unknown")),
        "boot_id": meta.get("boot_id", obj.get("boot_id")),
        "sequence": meta.get("sequence", obj.get("seq", obj.get("sequence"))),
        "event_utc": meta.get("event_utc", obj.get("event_utc")),
        "uptime_ms": obj.get("uptime_ms", meta.get("uptime_ms")),
        "timestamp_utc_ms": obj.get("timestamp_utc_ms", meta.get("timestamp_utc_ms")),
        "count": actual,
        "quality": (obj.get("quality") if isinstance(obj.get("quality"), dict)
                    else {k: obj[k] for k in ("rssi_dbm", "lqi", "frequency_offset_hz")
                          if k in obj}),
        **normalized,
    }
    if not isinstance(capture["capture_phase"], str):
        raise ReplayError("invalid capture phase")
    # Hash links local test runs without displaying raw RF payload or IDs.
    token = json.dumps(
        {"radio": radio, "format": fmt, "data": normalized,
         "boot_id": capture["boot_id"], "sequence": capture["sequence"],
         "event_utc": capture["event_utc"], "capture_phase": capture["capture_phase"]},
        sort_keys=True, separators=(",", ":"),
    )
    capture["capture_id"] = hashlib.sha256(token.encode("utf-8")).hexdigest()[:20]
    return capture


def load_captures(path: Path):
    if not path.is_file():
        raise ReplayError("input file does not exist")
    if path.stat().st_size > MAX_CSV_BYTES:
        raise ReplayError("input exceeds 96 MiB safety limit")
    counters = collections.Counter()
    issues = []
    captures = []

    def ingest(raw, meta=None):
        if len(captures) >= MAX_CAPTURES:
            counters["capture_limit_reached"] += 1
            return
        try:
            captures.append(_parse_capture(raw, meta))
        except ReplayError as error:
            counters["invalid_captures"] += 1
            if len(issues) < 15:
                issues.append(str(error))  # never echo sensitive data

    if path.suffix.lower() == ".csv":
        try:
            with path.open("r", encoding="utf-8-sig", newline="") as stream:
                reader = csv.DictReader(stream)
                required = {"record_type", "detail_state", "detail_json", "radio", "code"}
                if not reader.fieldnames or not required.issubset(reader.fieldnames):
                    raise ReplayError("not an RFLink full diagnostics CSV")
                for row in reader:
                    if row.get("record_type") != "event":
                        continue
                    counters["event_rows"] += 1
                    if row.get("detail_state") != "retained" or not row.get("detail_json"):
                        if row.get("code") in ("1", "2", "3", "4", "5", "6", "46"):
                            counters["missing_or_expired_rf_details"] += 1
                        else:
                            counters["events_without_rf_details"] += 1
                        continue
                    if row.get("detail_type") not in ("fifo_bytes_hex", "signed_pulses_us"):
                        counters["non_rf_details"] += 1
                        continue
                    try:
                        raw = json.loads(row["detail_json"])
                    except (ValueError, TypeError):
                        counters["invalid_captures"] += 1
                        continue
                    ingest(raw, {
                        "radio": row["radio"],
                        "boot_id": row.get("boot_id") or None,
                        "sequence": row.get("sequence") or None,
                        "event_utc": row.get("event_utc") or None,
                        "uptime_ms": row.get("uptime_ms") or None,
                    })
        except (UnicodeError, csv.Error) as error:
            raise ReplayError("unable to parse CSV: " + str(error)) from error
    elif path.suffix.lower() == ".json":
        try:
            document = json.loads(path.read_text(encoding="utf-8-sig"))
        except (ValueError, UnicodeError) as error:
            raise ReplayError("unable to parse JSON: " + str(error)) from error
        if isinstance(document, dict) and document.get("schema") == BUNDLE_SCHEMA:
            records = document.get("captures")
            if not isinstance(records, list):
                raise ReplayError("replay bundle must contain captures array")
        elif isinstance(document, list):
            records = document
        else:
            records = [document]
        for row in records:
            counters["json_rows"] += 1
            ingest(row, row if isinstance(row, dict) else None)
    else:
        raise ReplayError("input must be .csv or .json")
    return captures, dict(counters), issues


def _timing_pairs(pulses):
    # Exclude an idle delimiter only if it is unmistakably longer than a
    # regular RF symbol. We never infer missing portions of a capture.
    values = list(pulses)
    if values and abs(values[-1]) >= 10000:
        values.pop()
    if len(values) < 16:
        return [], "insufficient_signed_pulses"
    if sum((a > 0) != (b > 0) for a, b in zip(values, values[1:])) < len(values) - 2:
        return [], "not_alternating_high_low"
    high = [abs(x) for x in values if x > 0]
    low = [abs(x) for x in values if x < 0]
    if min(len(high), len(low)) < 8:
        return [], "not_enough_pairs"

    def cv(nums):
        m = statistics.mean(nums)
        return statistics.pstdev(nums) / m if m else 1.0

    candidates = []
    for variable, constant, kind in (
        (high, low, "pulse_width_candidate"),
        (low, high, "pulse_distance_candidate"),
    ):
        # A constant opposite phase distinguishes width/space modulation
        # from unrelated wideband timing noise.
        if cv(constant) > 0.24:
            continue
        lo, hi = min(variable), max(variable)
        if hi / lo < 1.5 or hi / lo > 8:
            continue
        centers = [float(lo), float(hi)]
        for _ in range(9):
            groups = [[], []]
            for v in variable:
                j = int(abs(v - centers[1]) < abs(v - centers[0]))
                groups[j].append(v)
            if not all(len(g) >= 3 for g in groups):
                break
            centers = [statistics.median(g) for g in groups]
        else:
            if centers[1] / centers[0] < 1.5 or any(cv(g) > 0.23 for g in groups):
                continue
            threshold = (centers[0] + centers[1]) / 2
            bits = "".join("1" if v > threshold else "0" for v in variable)
            candidates.append({
                "kind": kind,
                "bits": bits, "bit_count": len(bits),
                "short_us": round(centers[0], 1),
                "long_us": round(centers[1], 1),
                "confidence": "timing_hypothesis_only",
                "protocol_verified": False,
            })

    # Manchester candidate: reconstruct half-symbol cells, validate each pair
    # is complementary. Both polarities are reported because wire polarity
    # is not established by an isolated RF pulse trace.
    widths = [abs(x) for x in values]
    base = statistics.median(sorted(widths)[:max(4, len(widths) // 4)])
    if base:
        cells = []
        total_error = 0.0
        for duration in values:
            multiples = round(abs(duration) / base)
            err = abs(abs(duration) - multiples * base) / base
            if multiples not in (1, 2) or err > 0.22:
                cells = []
                break
            cells.extend("1" if duration > 0 else "0" for _ in range(multiples))
            total_error += err
        if len(cells) >= 32 and len(cells) <= 4096:
            for offset in (0, 1):
                pairs = [(cells[i], cells[i + 1]) for i in range(offset, len(cells) - 1, 2)]
                if len(pairs) >= 16 and all(a != b for a, b in pairs):
                    bits = "".join(a for a, _ in pairs)
                    candidates.append({
                        "kind": "manchester_candidate",
                        "bits": bits, "inverted_bits": "".join("1" if b == "0" else "0" for b in bits),
                        "bit_count": len(bits), "half_bit_us": round(base, 1),
                        "phase_offset": offset, "confidence": "timing_hypothesis_only",
                        "protocol_verified": False,
                    })
                    break
    return candidates, "candidate_timings_only" if candidates else "no_stable_timing_candidate"


def _compile_native(tmp: Path):
    # The HA Builder image compiles this exact helper at IMAGE BUILD time.
    # Only Builder-controlled environment can supply this local executable;
    # no untrusted CSV/JSON input may choose a binary or shell command.
    bundled = os.environ.get("RFLINK_RF_REPLAY_CODECS_BIN", "")
    if bundled:
        binary = Path(bundled)
        if not binary.is_file() or not os.access(binary, os.X_OK):
            raise ReplayError("the trusted Builder replay codec binary is unavailable")
        return binary
    compiler = shutil.which("g++") or shutil.which("clang++")
    if compiler is None:
        raise ReplayError(
            "native 868 decoding requires g++ or clang++; use --structural-only "
            "to parse captures without claiming protocol matches")
    inc = ROOT / "firmware/components/rflink_decoder_core/include"
    src = ROOT / "firmware/components/rflink_decoder_core/src"
    exe = tmp / ("rf_replay_codecs.exe" if sys.platform == "win32" else "rf_replay_codecs")
    cmd = [compiler, "-std=c++17", "-O2", "-I", str(inc),
           str(ROOT / "tools/rf_replay_codecs.cpp"),
           str(src / "rflink_ramses_codec.cpp"),
           str(src / "rflink_itho_codec.cpp"), "-o", str(exe)]
    result = subprocess.run(cmd, text=True, capture_output=True, timeout=90, check=False)
    if result.returncode != 0:
        raise ReplayError("unable to build native 868 codecs: " + result.stderr[-900:])
    return exe


def replay(captures, structural_only=False):
    verdicts = [None] * len(captures)
    fsk_indices = [i for i, cap in enumerate(captures) if cap["format"] == "fifo_bytes_hex"]
    if fsk_indices and not structural_only:
        with tempfile.TemporaryDirectory(prefix="rflink-replay-codecs-") as d:
            exe = _compile_native(Path(d))
            input_lines = "\n".join(captures[i]["hex"] for i in fsk_indices) + "\n"
            decoded = subprocess.run(
                [str(exe)], input=input_lines, text=True,
                capture_output=True, timeout=120, check=False)
            if decoded.returncode != 0:
                raise ReplayError("native 868 decoder process failed")
            lines = decoded.stdout.splitlines()
            if len(lines) != len(fsk_indices):
                raise ReplayError("native 868 decoder returned wrong result count")
            try:
                for i, line in zip(fsk_indices, lines):
                    verdicts[i] = json.loads(line)
            except (ValueError, TypeError) as error:
                raise ReplayError("native codec returned invalid JSON") from error
    for i, cap in enumerate(captures):
        if cap["format"] == "signed_pulses_us":
            candidates, reason = _timing_pairs(cap["pulses_us"])
            verdicts[i] = {
                "protocol": None, "reason": reason, "candidates": candidates,
                "protocol_verified": False,
            }
        elif verdicts[i] is None:
            verdicts[i] = {
                "protocol": None, "reason": "native_decoder_not_run",
                "hardware_crc_verified": False,
            }
    outputs = []
    for cap, verdict in zip(captures, verdicts):
        out = {key: cap.get(key) for key in (
            "capture_id", "radio", "frequency_hz", "format", "capture_phase",
            "boot_id", "sequence", "event_utc", "uptime_ms", "timestamp_utc_ms", "count")}
        out["quality"] = cap["quality"]
        out["result"] = verdict
        outputs.append(out)
    return outputs


def make_report(captures, counters, issues, structural_only=False):
    outputs = replay(captures, structural_only=structural_only)
    summary = collections.Counter()
    for rec in outputs:
        summary[f"radio_{rec['radio']}"] += 1
        summary[rec["result"]["reason"]] += 1
        if rec["result"]["protocol"]:
            summary[f"protocol_{rec['result']['protocol']}"] += 1
        if rec["result"].get("candidates"):
            summary["433_with_timing_hypotheses"] += 1
    return {
        "schema": SCHEMA,
        "capture_count": len(captures),
        "input_counters": counters,
        "warnings": issues,
        "decoder_mode": "structural_only" if structural_only else "native_firmware_codecs",
        "summary": dict(sorted(summary.items())),
        "results": outputs,
        "notes": [
            "Only data retained in the source export can be replayed.",
            "433 bit patterns are timing hypotheses, never validated protocols.",
            "868 protocol checks reuse firmware RAMSES-II/Itho built-in codecs; installed user recipes are not loaded.",
            "The current 868 CC1101 RX profile has hardware CRC disabled; LQI is not CRC verification.",
            "No radio hardware is accessed and no RF transmission is performed.",
            "Reports omit raw RF values and device identities; optional capture bundles contain sensitive bytes.",
        ],
    }


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, help="RFLink_Diagnostics_Full.csv, RF-capture JSON or capture bundle")
    parser.add_argument("--report", type=Path, help="write sanitized replay report JSON")
    parser.add_argument("--save-captures", type=Path, help="write PRIVATE normalized RAW capture bundle for later replay")
    parser.add_argument("--structural-only", action="store_true",
                        help="skip native codec compilation (no 868 protocol verdict)")
    args = parser.parse_args(argv)
    try:
        captures, counters, issues = load_captures(args.input)
        report = make_report(captures, counters, issues, args.structural_only)
        if args.report:
            args.report.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n",
                                   encoding="utf-8")
        if args.save_captures:
            private = {"schema": BUNDLE_SCHEMA, "captures": captures}
            args.save_captures.write_text(
                json.dumps(private, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
            try:
                args.save_captures.chmod(0o600)
            except OSError:
                pass
        print(json.dumps({
            "schema": SCHEMA, "capture_count": len(captures),
            "summary": report["summary"], "input_counters": counters,
            "warning_count": len(issues), "decoder_mode": report["decoder_mode"],
        }, ensure_ascii=False, indent=2))
        return 0
    except (ReplayError, OSError, subprocess.TimeoutExpired) as error:
        print("RFLink offline replay: " + str(error), file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
