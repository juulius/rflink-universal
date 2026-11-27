#!/usr/bin/env python3
"""Offline RFLink replay v1: 433 signed RMT pulses + 868 FIFO and isolated clocked windows.

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
if __package__:
    from .rf_signal_families import analyze_signal_families
else:
    from rf_signal_families import analyze_signal_families

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
    # Log Viewer exports an envelope; the raw object contains the waveform.
    if obj.get("schema") == "rflink-rf-capture-v1":
        if not isinstance(obj.get("capture"), dict):
            raise ReplayError("RF capture envelope has no capture object")
        meta = {**meta, "capture_phase": obj.get("type", "unknown")}
        obj = obj["capture"]
    radio = _integer(obj.get("radio", meta.get("radio")), "radio", 0, 2)
    frequency = _integer(obj.get("frequency_hz", FREQ[radio]), "frequency_hz")
    if abs(frequency - FREQ[radio]) > 200000:
        raise ReplayError("frequency does not match radio")
    fmt = obj.get("format") or obj.get("detail_type")
    if fmt not in ("fifo_bytes_hex", "signed_pulses_us", "clocked_bits_hex", "carrier_bits_hex"):
        raise ReplayError("missing or unsupported capture format")
    if fmt in ("fifo_bytes_hex", "clocked_bits_hex", "carrier_bits_hex") and radio != 2:
        raise ReplayError("FIFO format cannot be assigned to a 433 RMT radio")
    if fmt == "signed_pulses_us" and radio == 2:
        raise ReplayError("RMT pulse format cannot be assigned to the 868 FIFO radio")
    if fmt in ("fifo_bytes_hex", "clocked_bits_hex", "carrier_bits_hex"):
        raw = obj.get("hex", obj.get("fifo_bytes_hex"))
        if not isinstance(raw, str) or not raw or not HEX_RE.fullmatch(raw):
            raise ReplayError("invalid FIFO hex")
        data = bytes.fromhex(raw)
        if fmt == "clocked_bits_hex" and _integer(obj.get("bit_count"), "bit count") != 504:
            raise ReplayError("automatic clocked capture must contain 504 bits")
        if fmt == "carrier_bits_hex":
            if not 1 <= len(data) <= 384 or _integer(obj.get("bit_count"), "bit count") != len(data)*8:
                raise ReplayError("carrier window must contain 1..384 complete bytes")
        elif len(data) != 63:
            raise ReplayError("868 FIFO capture must contain exactly 63 bytes")
        normalized = {"hex": data.hex().upper()}
        actual = len(data)
        if fmt in ("clocked_bits_hex", "carrier_bits_hex"):
            normalized["bit_count"] = len(data)*8
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
    # Preserve only bounded numeric/enum observation fields, never arbitrary
    # imported dictionaries or a new interpretation of absent measurements.
    for field, keys in (
        ("observation", ("total_us","min_us","max_us","high","low","zero",
                         "short_durations","level_changes","timing_samples","outside_bins",
                         "top_two_support","high_modes","low_modes","bits","ones","transitions",
                         "alternating_prefix","longest_run","tail_bits","tail_ones",
                         "tail_transitions","tail_longest_run")),
        ("carrier_context", ("epoch","estimated_start_us","estimated_end_us","samples",
                             "carrier_samples","missed_samples","first_sample_us","last_sample_us",
                             "min_rssi_dbm_x2","max_rssi_dbm_x2","first_burst","last_burst"))):
        source = obj.get(field)
        if isinstance(source, dict):
            safe = {k: source[k] for k in keys
                    if type(source.get(k)) is int and -(2**53-1) <= source[k] <= 2**53-1}
            for k in ("state","pattern","kind","schema","time_basis"):
                if source.get(k) in ("not_recorded","invalid_timing","history_unavailable",
                    "no_matching_samples","sampled","insufficient_timing","concentrated_timing",
                    "varied_timing","insufficient_tail","long_constant_run","predominantly_alternating",
                    "mixed_bits","pulse_timing","clocked_bits","rflink-signal-observation-v1",
                    "rflink-carrier-context-v1","interpolated_block_endpoints"):
                    safe[k] = source[k]
            if type(source.get("history_starts_late")) is bool:
                safe["history_starts_late"] = source["history_starts_late"]
            capture[field] = safe
    if fmt == "carrier_bits_hex":
        for k, upper in (("activity_id", 2**32-1), ("sampled_span_us", 1000000)):
            if k in obj:
                capture[k] = _integer(obj[k], k, 0, upper)
        for k in ("short_history", "limited"):
            if type(obj.get(k)) is bool:
                capture[k] = obj[k]
    if fmt == "signed_pulses_us" and "decoder_input_count" in obj:
        capture["decoder_input_count"] = _integer(obj["decoder_input_count"], "decoder input count", 0, 2048)
    if not isinstance(capture["capture_phase"], str):
        raise ReplayError("invalid capture phase")
    # Capture identity must stay stable when SNTP/boot-epoch estimates move
    # between two exports of the SAME stored event. The ESP32 reconstructs
    # event_utc/timestamp_utc_ms from uptime at export time, so neither UTC
    # value belongs in the hash used for A/B regression matching.
    # Preserve stable boot/sequence/uptime identity and exact RF content.
    def stable_meta(value):
        if value is None or value == "":
            return None
        if isinstance(value, (str, int)) and not isinstance(value, bool):
            text = str(value).strip()
            return text or None
        return None

    token = json.dumps(
        {"radio": radio, "frequency_hz": frequency, "format": fmt,
         "data": normalized, "boot_id": stable_meta(capture["boot_id"]),
         "sequence": stable_meta(capture["sequence"]),
         "uptime_ms": stable_meta(capture["uptime_ms"]),
         "capture_phase": capture["capture_phase"]},
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
                        if row.get("code") in ("1", "2", "3", "4", "5", "6", "7", "8", "46", "55", "57"):
                            counters["missing_or_expired_rf_details"] += 1
                        else:
                            counters["events_without_rf_details"] += 1
                        continue
                    if row.get("detail_type") not in ("fifo_bytes_hex", "signed_pulses_us", "clocked_bits_hex", "carrier_bits_hex"):
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
           "-I", str(ROOT / "firmware/main"),
           "-I", str(ROOT / "firmware/components/rflink_radio_hal/include"),
           str(ROOT / "firmware/main/rflink_sync868_replay.cpp"),
           str(ROOT / "tools/rf_replay_codecs.cpp"),
           str(src / "rflink_ramses_codec.cpp"),
           str(src / "rflink_itho_codec.cpp"), "-o", str(exe)]
    result = subprocess.run(cmd, text=True, capture_output=True, timeout=90, check=False)
    if result.returncode != 0:
        raise ReplayError("unable to build native 868 codecs: " + result.stderr[-900:])
    return exe


# 433 evidence clustering is intentionally conservative. A similar timing shape
# is only a candidate repetition, never a decoded RF protocol or device ID.
# Output omits all underlying signed pulse arrays and RF packet bytes.
MAX_CLUSTER_COMPARISONS = 64
MIN_CLUSTER_DURATIONS = 24


def _number_or_none(value):
    try:
        return None if value is None or isinstance(value, bool) else int(value)
    except (ValueError, TypeError, OverflowError):
        return None


def _433_trim_idle(pulses):
    # Native decoder inputs can end with a synthetic ~5ms idle delimiter.
    # Do not remove an ordinary long on-air period or a lone short sample.
    if len(pulses) >= MIN_CLUSTER_DURATIONS + 1:
        tail = abs(pulses[-1])
        baseline = statistics.median(abs(v) for v in pulses[:-1])
        if tail >= 4000 and baseline > 0 and tail >= baseline * 4:
            return pulses[:-1]
    return pulses


def _433_shape(pulses):
    """Candidate waveform for grouping, excluding obviously incomplete bursts."""
    values = _433_trim_idle(pulses)
    if len(values) < MIN_CLUSTER_DURATIONS:
        return None
    if sum((x > 0) != (y > 0) for x, y in zip(values, values[1:])) < len(values) - 2:
        return None
    if min(sum(x > 0 for x in values), sum(x < 0 for x in values)) < 10:
        return None
    magnitudes = tuple(abs(x) for x in values)
    scale = statistics.median(magnitudes)
    if scale <= 0:
        return None
    # Fixing the overall scale prevents classifying signals with very different
    # timings as the same just because the relative waveform looks alike.
    return (tuple(1 if x > 0 else -1 for x in values), magnitudes, scale)


def _433_similar(a, b):
    signs_a, magnitudes_a, scale_a = a
    signs_b, magnitudes_b, scale_b = b
    if signs_a != signs_b or len(magnitudes_a) != len(magnitudes_b):
        return False
    if not 0.82 <= scale_a / scale_b <= 1.22:
        return False
    # Cheap indexed check before a full bounded comparison.
    n = len(magnitudes_a)
    for i in (0, n // 4, n // 2, 3 * n // 4, n - 1):
        ratio = magnitudes_a[i] / magnitudes_b[i]
        if not 0.68 <= ratio <= 1.48:
            return False
    errors = [abs(x - y) / max(x, y) for x, y in zip(magnitudes_a, magnitudes_b)]
    # Median handles a small amount of jitter. The 90th percentile guards
    # against unrelated messages with a matching length and average width.
    errors.sort()
    return errors[len(errors) // 2] <= 0.17 and errors[int((len(errors) - 1) * 0.9)] <= 0.32


def _433_waveform_diagnostics(pulses):
    """Bounded summary of recorded signed durations, never the raw waveform.

    High/low dispersion is descriptive only: different values can encode
    legitimate data bits and must NOT be labelled corruption or RF jitter.
    """
    raw = list(pulses)
    cleaned = _433_trim_idle(raw)
    levels = [1 if x > 0 else -1 for x in cleaned]
    widths = sorted(abs(x) for x in cleaned)
    positive = [x for x in cleaned if x > 0]
    negative = [-x for x in cleaned if x < 0]
    count = len(cleaned)
    alternation_breaks = sum(a == b for a, b in zip(levels, levels[1:]))
    if count < MIN_CLUSTER_DURATIONS:
        grouping_reason = "too_few_durations"
    elif alternation_breaks > 1:
        grouping_reason = "non_alternating_phases"
    elif min(len(positive), len(negative)) < 10:
        grouping_reason = "insufficient_high_low"
    elif not statistics.median(widths):
        grouping_reason = "no_timing_scale"
    else:
        grouping_reason = "eligible"
    def cv(values):
        return round(statistics.pstdev(values) / statistics.mean(values), 3) if values else None
    def pct(part):
        return widths[int((count - 1) * part)] if count else 0
    return {
        "duration_count_without_idle_candidate": count,
        "terminal_idle_candidate_removed": len(raw) != len(cleaned),
        "high_durations": len(positive),
        "low_durations": len(negative),
        "alternation_breaks": alternation_breaks,
        "min_duration_us": widths[0] if count else 0,
        "median_duration_us": round(statistics.median(widths), 1) if count else 0,
        "p90_duration_us": pct(0.9),
        "max_duration_us": widths[-1] if count else 0,
        "durations_under_80us": sum(v < 80 for v in widths),
        "durations_over_4000us": sum(v >= 4000 for v in widths),
        "high_variation_cv": cv(positive),
        "low_variation_cv": cv(negative),
        "grouping_eligibility": grouping_reason,
    }


def analyze_433_evidence(captures, outputs):
    """Link adjacent pre/post-filter records and group conservative repeats.

    Pairing requires consecutive event sequences in the *same* boot/radio and
    matching timestamps; it never guesses links from pulse lengths alone.
    Grouping uses one representative physical frame per pair (post-filter
    preferred) to avoid treating two views of one capture as two RF signals.
    """
    summary = collections.Counter()
    per_radio = {0: collections.Counter(), 1: collections.Counter()}
    pending = {}
    paired = {}
    # Annotate both capture phases, not only the decoder representative.
    # Comparing their aggregate morphology is useful for testing filters
    # without exposing the signed duration sequence in sanitized reports.
    for i, cap in enumerate(captures):
        if cap["radio"] in (0, 1) and cap["format"] == "signed_pulses_us":
            outputs[i]["rf433_waveform"] = _433_waveform_diagnostics(cap["pulses_us"])
    for i, cap in enumerate(captures):
        if cap["radio"] not in (0, 1) or cap["format"] != "signed_pulses_us":
            continue
        phase = cap.get("capture_phase")
        radio = cap["radio"]
        if phase == "pre_filter_rmt":
            if "decoder_input_count" in cap:
                # Consolidated captures already represent one physical event.
                pending.pop(radio, None)
                outputs[i]["rf433_evidence"] = {
                    "pre_filter_durations": cap["count"],
                    "decoder_input_durations": cap["decoder_input_count"],
                    "duration_count_delta": cap["decoder_input_count"]-cap["count"],
                    "pairing": "consolidated_metadata_only",
                }
                continue
            pending[radio] = i
            continue
        if phase != "decoder_input":
            pending.pop(radio, None)
            continue
        pre_index = pending.pop(radio, None)
        if pre_index is None:
            continue
        pre = captures[pre_index]
        pre_seq, post_seq = _number_or_none(pre.get("sequence")), _number_or_none(cap.get("sequence"))
        pre_uptime, post_uptime = _number_or_none(pre.get("uptime_ms")), _number_or_none(cap.get("uptime_ms"))
        pre_timestamp, post_timestamp = _number_or_none(pre.get("timestamp_utc_ms")), _number_or_none(cap.get("timestamp_utc_ms"))
        same_clock = (pre_uptime is not None and pre_uptime == post_uptime or
                      pre_timestamp is not None and pre_timestamp == post_timestamp)
        if (pre_seq is None or post_seq != pre_seq + 1 or not same_clock or
                pre.get("boot_id") != cap.get("boot_id") or
                pre["frequency_hz"] != cap["frequency_hz"]):
            continue
        paired[pre_index] = i
        paired[i] = pre_index
        pre_count, post_count = pre["count"], cap["count"]
        pre_wave = outputs[pre_index]["rf433_waveform"]
        post_wave = outputs[i]["rf433_waveform"]
        observation = {
            "pre_filter_durations": pre_count,
            "decoder_input_durations": post_count,
            "duration_count_delta": post_count - pre_count,
            "pre_filter_median_duration_us": pre_wave["median_duration_us"],
            "decoder_input_median_duration_us": post_wave["median_duration_us"],
            "pre_filter_short_durations": pre_wave["durations_under_80us"],
            "decoder_input_short_durations": post_wave["durations_under_80us"],
            "paired_capture_id": outputs[i]["capture_id"],
            "pairing": "adjacent_sequence_same_boot_radio_and_time",
        }
        outputs[pre_index]["rf433_evidence"] = observation
        outputs[i]["rf433_evidence"] = {
            **observation, "paired_capture_id": outputs[pre_index]["capture_id"]
        }
        summary["paired_pre_post_filter"] += 1
        per_radio[radio]["paired_pre_post_filter"] += 1
        if pre_count != post_count:
            summary["pairs_with_duration_count_change"] += 1
            per_radio[radio]["pairs_with_duration_count_change"] += 1

    clusters_by_shape = {}
    eligible_buckets_by_radio = {0: collections.Counter(), 1: collections.Counter()}
    clusters = []
    for i, cap in enumerate(captures):
        if cap["radio"] not in (0, 1) or cap["format"] != "signed_pulses_us":
            continue
        if cap.get("capture_phase") == "pre_filter_rmt" and i in paired:
            continue   # a paired decoder input is the single representative
        summary["physical_capture_candidates"] += 1
        band = per_radio[cap["radio"]]
        band["physical_capture_candidates"] += 1
        waveform = outputs[i]["rf433_waveform"]
        bucket_reason = waveform["grouping_eligibility"]
        band[bucket_reason] += 1
        if waveform["terminal_idle_candidate_removed"]:
            band["terminal_idle_candidates_removed"] += 1
        if waveform["durations_under_80us"]:
            band["frames_with_under_80us"] += 1
        if waveform["durations_over_4000us"]:
            band["frames_with_over_4000us"] += 1
        band["max_duration_count"] = max(band["max_duration_count"], cap["count"])
        if cap["count"] < 16:
            band["count_1_to_15"] += 1
        elif cap["count"] < 24:
            band["count_16_to_23"] += 1
        elif cap["count"] < 64:
            band["count_24_to_63"] += 1
        else:
            band["count_64_plus"] += 1
        shape = _433_shape(cap["pulses_us"])
        if shape is None:
            summary["too_short_or_irregular_for_grouping"] += 1
            continue
        summary["eligible_for_grouping"] += 1
        radio, n = cap["radio"], len(shape[0])
        eligible_buckets_by_radio[radio][(n, shape[0][0])] += 1
        bucket = clusters_by_shape.setdefault((radio, n, shape[0][0]), [])
        found = None
        for index in bucket[:MAX_CLUSTER_COMPARISONS]:
            per_radio[radio]["same_length_similarity_checks"] += 1
            if _433_similar(shape, clusters[index]["_shape"]):
                per_radio[radio]["same_length_similarity_matches"] += 1
                found = clusters[index]
                break
            per_radio[radio]["same_length_similarity_rejections"] += 1
        if found is None:
            # A capped comparison budget prevents an RF noise storm from
            # causing unbounded O(n² × pulse_count) work in HA Builder.
            if len(bucket) >= MAX_CLUSTER_COMPARISONS:
                summary["comparison_budget_exhausted"] += 1
                continue
            found = {
                "group_id": "G" + str(len(clusters) + 1),
                "radio": radio,
                "count": 0,
                "first_event_utc": cap.get("event_utc"),
                "last_event_utc": None,
                "representative_capture_id": outputs[i]["capture_id"],
                "sample_capture_ids": [],
                "protocol_verified": False,
                "interpretation": "similar_pulse_timing_only",
                "_shape": shape,
            }
            clusters.append(found)
            bucket.append(len(clusters) - 1)
        found["count"] += 1
        found["last_event_utc"] = cap.get("event_utc")
        if len(found["sample_capture_ids"]) < 5:
            found["sample_capture_ids"].append(outputs[i]["capture_id"])
        # Attach group to both pre/post rows but count the frame only once.
        group_members = (i, paired[i]) if i in paired else (i,)
        for member in group_members:
            outputs[member].setdefault("rf433_evidence", {})["group_id"] = found["group_id"]
            outputs[member]["rf433_evidence"]["grouping"] = "unverified_timing_similarity"

    # Expose whether timing candidates can even be compared under our exact
    # length and starting-polarity contract. Zero matches can simply mean
    # all captures have different lengths, not that the RF is random noise.
    for radio in (0, 1):
        buckets = eligible_buckets_by_radio[radio]
        per_radio[radio]["distinct_length_polarity_buckets"] = len(buckets)
        per_radio[radio]["frames_with_length_polarity_peer"] = sum(
            count for count in buckets.values() if count >= 2)
        per_radio[radio]["length_polarity_buckets_with_peers"] = sum(
            count >= 2 for count in buckets.values())

    # Group IDs are temporary bucket identifiers. A single pre/post
    # capture pair is ONE physical-frame candidate, not a repeat. Suppress
    # singleton "group" labels on rows to prevent false repeat claims.
    group_sizes = {g["group_id"]: g["count"] for g in clusters}
    for out in outputs:
        evidence = out.get("rf433_evidence")
        if not evidence or "group_id" not in evidence:
            continue
        size = group_sizes.get(evidence["group_id"], 0)
        if size < 2:
            evidence.pop("group_id", None)
            evidence.pop("grouping", None)
        else:
            evidence["group_count"] = size

    summary["groups_found_including_singletons"] = len(clusters)
    repeating = [g for g in clusters if g["count"] >= 2]
    summary["repeated_groups"] = len(repeating)
    summary["frames_in_repeated_groups"] = sum(g["count"] for g in repeating)
    # Never include the normalized waveform or packet content in a report.
    repeating.sort(key=lambda group: (-group["count"], group["group_id"]))
    summary["repeated_groups_in_report"] = min(40, len(repeating))
    return {
        "version": 1,
        "interpretation": "timing_similarity_only_not_a_verified_protocol_or_device",
        "summary": dict(summary),
        "per_radio_waveform": {
            "radio_0": dict(sorted(per_radio[0].items())),
            "radio_1": dict(sorted(per_radio[1].items())),
        },
        "repeated_groups": [
            {key: value for key, value in group.items() if key != "_shape"}
            for group in repeating[:40]
        ],
        "notes": [
            "Pre-filter and decoder input are correlated only on adjacent sequence, same radio/boot/time.",
            "Filtering may merge durations and append an artificial idle tail; delta is NOT a count of physical RF pulses lost.",
            "Short, unpaired or noise bursts are not classified as devices.",
            "Timing grouping is conservative, not a valid protocol, device ID or receiver-quality verdict.",
            "Duration histograms and high/low variation describe recorded pulse shapes, not bit errors or signal quality.",
        ],
    }


def replay(captures, structural_only=False):
    verdicts = [None] * len(captures)
    fsk_indices = [i for i, cap in enumerate(captures) if cap["format"] in ("fifo_bytes_hex", "clocked_bits_hex")]
    if fsk_indices and not structural_only:
        with tempfile.TemporaryDirectory(prefix="rflink-replay-codecs-") as d:
            exe = _compile_native(Path(d))
            input_lines = "\n".join(("SYNC:" if captures[i]["format"] == "clocked_bits_hex" else "") + captures[i]["hex"] for i in fsk_indices) + "\n"
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
        elif cap["format"] == "carrier_bits_hex":
            verdicts[i] = {"protocol": None, "reason": "carrier_window_observation_only", "protocol_verified": False}
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
        for field in ("bit_count", "observation", "carrier_context", "decoder_input_count", "activity_id", "sampled_span_us", "short_history", "limited"):
            if field in cap:
                out[field] = cap[field]
        out["result"] = verdict
        outputs.append(out)
    return outputs


def analyze_sync_evidence(captures, outputs):
    """Exact bit-window equality only; no fuzzy stitching or payload merging."""
    counts = collections.Counter()
    groups = {}
    for cap, out in zip(captures, outputs):
        if cap["format"] not in ("clocked_bits_hex", "carrier_bits_hex"):
            continue
        counts["windows"] += 1
        data = bytes.fromhex(cap["hex"])
        bits = "".join(f"{b:08b}" for b in data)
        tail = bits[32:] if cap["format"] == "clocked_bits_hex" else bits
        longest = max((len(m.group(0)) for m in re.finditer(r"0+|1+", tail)), default=0)
        if cap["format"] == "carrier_bits_hex":
            counts["carrier_windows"] += 1
        prefix = 1
        while prefix < len(bits) and bits[prefix] != bits[prefix-1]:
            prefix += 1
        c = cap.get("carrier_context", {})
        state = c.get("state", "not_recorded")
        if state not in ("sampled","not_recorded","invalid_timing","history_unavailable","no_matching_samples"):
            state = "not_recorded"
        counts["carrier_"+state] += 1
        if state == "sampled" and c.get("carrier_samples",0)>0:
            counts["windows_with_carrier_high"] += 1
        out["rf868_evidence"] = {
            "bit_count":len(bits), "alternating_prefix":prefix,
            "tail_ones":tail.count("1"),
            "tail_transitions":sum(a!=b for a,b in zip(tail,tail[1:])),
            "tail_longest_run":longest, "carrier_state":state,
        }
        # Include frequency, polarity and every bit. Keep every result row.
        group = groups.setdefault((cap["frequency_hz"],cap["format"],data), {})
        group.setdefault(cap["capture_id"], []).append(out)
    repeated = []
    for group in groups.values():
        if len(group) < 2:
            continue
        ident = "S"+str(len(repeated)+1)
        for entries in group.values():
            for out in entries:
                out["rf868_evidence"].update(group_id=ident,group_count=len(group))
        repeated.append({"group_id":ident,"count":len(group),"protocol_verified":False,
                         "interpretation":"identical_windows_distinct_capture_ids"})
    counts["exact_repeat_groups"] = len(repeated)
    counts["windows_in_repeat_groups"] = sum(g["count"] for g in repeated)
    repeated.sort(key=lambda g:(-g["count"],g["group_id"]))
    return {"version":1,"summary":dict(counts),"repeated_groups":repeated[:40]}


def make_report(captures, counters, issues, structural_only=False):
    outputs = replay(captures, structural_only=structural_only)
    rf433_insights = analyze_433_evidence(captures, outputs)
    rf868_insights = analyze_sync_evidence(captures, outputs)
    signal_families = analyze_signal_families(captures, outputs)
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
        "capture_id_scheme": "stable-source-v2",
        "capture_count": len(captures),
        "input_counters": counters,
        "warnings": issues,
        "decoder_mode": "structural_only" if structural_only else "native_firmware_codecs",
        "summary": dict(sorted(summary.items())),
        "results": outputs,
        "rf433_insights": rf433_insights,
        "rf868_insights": rf868_insights,
        "signal_families": signal_families,
        "notes": [
            "Only data retained in the source export can be replayed.",
            "433 bit patterns are timing hypotheses, never validated protocols.",
            "868 protocol checks reuse firmware RAMSES-II/Itho built-in codecs; installed user recipes are not loaded.",
            "Automatic 504-bit windows are independent fragments; no stitching or complete-frame claim. Itho structural hits and embedded RAMSES matches are reported separately.",
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
