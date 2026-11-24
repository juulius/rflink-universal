#!/usr/bin/env python3
"""Validate and prepare user-exported RFLink synchronous DATA+CLOCK captures.

Inputs are **raw demodulated bits**, not validated Itho or RAMSES packets.
This script never configures a CC1101, transmits, contacts a server or
claims an RF protocol match. Use it as the loss-aware input for later
native offline protocol replay.

Usage:
  python firmware/tools/inspect_sync_bits.py RFLink_868_SyncBits_Bench.csv
  python firmware/tools/inspect_sync_bits.py RFLink_Diagnostics_Full.csv
  python firmware/tools/inspect_sync_bits.py capture.csv --segments-json segments.json

The optional JSON contains raw RF bits and may include device identifiers
and remote-control commands. Keep it private; never commit it to GitHub.
"""
from __future__ import annotations

import argparse
import csv
import json
from dataclasses import dataclass
from pathlib import Path
import re
import sys

SCHEMA = "rflink-sync-bits-v1"
HEADER = [
    "index", "epoch", "bit_count", "first_us", "last_us",
    "discontinuity_before", "window_end", "hex_msb",
]
MAX_BLOCKS = 256
MAX_BYTES = 1_000_000
MAX_FULL_CSV_BYTES = 96 * 1024 * 1024


@dataclass(frozen=True)
class BitBlock:
    index: int
    epoch: int
    bit_count: int
    first_us: int
    last_us: int
    discontinuity_before: bool
    window_end: bool
    hex_msb: str

    def bitstring(self) -> str:
        """Return only actual bits; ignore padding in the last byte."""
        return "".join(f"{n:08b}" for n in bytes.fromhex(self.hex_msb))[:self.bit_count]


def _positive_int(value: str, name: str, minimum: int = 0) -> int:
    if not value.isdecimal():
        raise ValueError(f"{name}: invalid nonnegative integer")
    number = int(value)
    if number < minimum:
        raise ValueError(f"{name}: value below {minimum}")
    return number


def read_full_diagnostics_capture(path: Path) -> tuple[dict[str, str], list[BitBlock]]:
    """Extract protected, frozen synchronous evidence from a unified v2 log CSV.

    Never infer signal frames from 868 clock counters. Validate original
    bit counts, padding and boundaries before sending them to the decoder.
    """
    if path.stat().st_size > MAX_FULL_CSV_BYTES:
        raise ValueError("Full diagnostic CSV exceeds the 96 MiB safety limit")
    blocks: list[BitBlock] = []
    context = None
    with path.open("r", encoding="utf-8-sig", newline="") as source:
        records = csv.DictReader(source)
        required = {"record_type", "kind", "radio", "detail_state",
                    "detail_type", "detail_json", "snapshot_json"}
        if not records.fieldnames or not required.issubset(records.fieldnames):
            raise ValueError("Not an RFLink full diagnostics CSV")
        for row in records:
            if row["record_type"] == "context" and row["kind"] == "sync868_capture":
                if context is not None:
                    raise ValueError("Duplicate 868 capture context")
                try:
                    context = json.loads(row["snapshot_json"])
                except (TypeError, ValueError) as exc:
                    raise ValueError("Invalid 868 capture context JSON") from exc
                if not isinstance(context, dict):
                    raise ValueError("Invalid 868 capture context structure")
                continue
            if row["record_type"] != "sync868_bit_block":
                continue
            if len(blocks) >= MAX_BLOCKS:
                raise ValueError("Full CSV exceeds the fixed 868 capture budget")
            if row["radio"] != "2" or row["detail_state"] != "retained" or (
                    row["detail_type"] != "sync_bits_msb_hex"):
                raise ValueError("Invalid 868 RAW row classification")
            try:
                raw = json.loads(row["detail_json"])
            except (TypeError, ValueError) as exc:
                raise ValueError("Invalid 868 RAW JSON row") from exc
            if not isinstance(raw, dict) or raw.get("schema") != SCHEMA:
                raise ValueError("Unexpected 868 RAW schema")
            integer_fields = ("generation", "index", "epoch", "bit_count",
                              "first_us", "last_us")
            if any(type(raw.get(k)) is not int or raw[k] < 0
                   for k in integer_fields):
                raise ValueError("Invalid 868 RAW numeric metadata")
            if raw["index"] != len(blocks) or raw["epoch"] < 1:
                raise ValueError("Out-of-order 868 RAW block or invalid epoch")
            bits = raw["bit_count"]
            if bits < 1 or bits > 2048 or raw["first_us"] > raw["last_us"]:
                raise ValueError("Invalid 868 RAW bit count or timestamps")
            if any(type(raw.get(k)) is not bool
                   for k in ("discontinuity_before", "window_end")):
                raise ValueError("Invalid 868 RAW boundary flags")
            hex_bits = raw.get("hex_msb")
            if not isinstance(hex_bits, str) or (
                    len(hex_bits) != 2 * ((bits + 7) // 8)) or (
                    not re.fullmatch(r"[0-9A-F]+", hex_bits)):
                raise ValueError("Invalid 868 RAW MSB-first hex payload")
            unused = (-bits) % 8
            if unused and (int(hex_bits[-2:], 16) & ((1 << unused) - 1)):
                raise ValueError("Nonzero 868 RAW padding bits")
            blocks.append(BitBlock(
                raw["index"], raw["epoch"], bits, raw["first_us"],
                raw["last_us"], raw["discontinuity_before"],
                raw["window_end"], hex_bits))
    if context is None:
        raise ValueError("Full diagnostic CSV has no 868 RAW capture context")
    if context.get("raw_embedded") is not True:
        raise ValueError("Full CSV has no protected 868 RAW bits. Freeze a capture, "
                         "enter the management key locally, then export again.")
    if type(context.get("generation")) is not int or context["generation"] < 1:
        raise ValueError("Invalid 868 RAW generation in context")
    if type(context.get("available_blocks")) is not int or (
            context["available_blocks"] != len(blocks)):
        raise ValueError("Full CSV 868 RAW block count mismatch")
    if type(context.get("recorded_bits")) is not int or (
            context["recorded_bits"] != sum(b.bit_count for b in blocks)):
        raise ValueError("Full CSV 868 RAW recorded bit count mismatch")
    if type(context.get("full")) is not bool:
        raise ValueError("Invalid 868 RAW full marker")
    # All blocks belong to the same explicitly frozen capture generation.
    # The outer context is independently generated from that capture lease.
    metadata = {"schema": SCHEMA, "mode": "BENCH_ONLY",
                "radio": "868.30 MHz",
                "generation": str(context["generation"]),
                "capacity": str(MAX_BLOCKS),
                "blocks": str(len(blocks)),
                "recorded_bits": str(context["recorded_bits"]),
                "full": "1" if context["full"] else "0"}
    # Read original JSON a second time only for <=256 bounded rows to check
    # generation consistency without retaining raw payload strings.
    with path.open("r", encoding="utf-8-sig", newline="") as source:
        for row in csv.DictReader(source):
            if row["record_type"] == "sync868_bit_block":
                raw = json.loads(row["detail_json"])
                if raw.get("generation") != context["generation"]:
                    raise ValueError("868 RAW capture generation changed during export")
    return metadata, blocks


def read_capture(path: Path) -> tuple[dict[str, str], list[BitBlock]]:
    # Both formats decode the identical bounded, loss-aware bit block records.
    with path.open("r", encoding="utf-8-sig") as source:
        heading = source.readline()
    if heading.startswith("record_type,") and "detail_json," in heading:
        return read_full_diagnostics_capture(path)
    if path.stat().st_size > MAX_BYTES:
        raise ValueError("CSV is larger than the 1 MB local bench safety limit")
    metadata: dict[str, str] = {}
    with path.open("r", encoding="utf-8-sig", newline="") as source:
        rows = csv.reader(source)
        headings = None
        for cells in rows:
            if not cells:
                continue
            if cells[0].startswith("#"):
                if len(cells) != 2:
                    raise ValueError("Malformed capture metadata")
                key = cells[0][1:]
                if key in metadata:
                    raise ValueError(f"Duplicate metadata key: {key}")
                metadata[key] = cells[1]
                continue
            headings = cells
            break
        if headings != HEADER:
            raise ValueError("Unknown CSV columns or missing capture header")
        if metadata.get("schema") != SCHEMA or metadata.get("mode") != "BENCH_ONLY":
            raise ValueError("Not an RFLink synchronous bench bit capture")
        if metadata.get("radio") != "868.30 MHz":
            raise ValueError("Unexpected radio/frequency metadata")
        capacity = _positive_int(metadata.get("capacity", ""), "capacity", 1)
        expected = _positive_int(metadata.get("blocks", ""), "blocks")
        total_bits = _positive_int(metadata.get("recorded_bits", ""), "recorded_bits")
        _positive_int(metadata.get("generation", ""), "generation", 1)
        if metadata.get("full") not in {"0", "1"}:
            raise ValueError("full must be 0 or 1")
        if capacity > MAX_BLOCKS or expected > capacity:
            raise ValueError("Capture exceeds declared fixed 256-block budget")
        blocks: list[BitBlock] = []
        for cells in rows:
            if len(cells) != len(HEADER):
                raise ValueError("Unexpected column count in a capture row")
            row = dict(zip(HEADER, cells))
            index = _positive_int(row["index"], "index")
            epoch = _positive_int(row["epoch"], "epoch", 1)
            bits = _positive_int(row["bit_count"], "bit_count", 1)
            first = _positive_int(row["first_us"], "first_us")
            last = _positive_int(row["last_us"], "last_us")
            if bits > 2048 or first > last or index != len(blocks):
                raise ValueError("Invalid bit count, timestamps, or block order")
            if row["discontinuity_before"] not in {"0", "1"} or row["window_end"] not in {"0", "1"}:
                raise ValueError("Capture boundary markers must be 0 or 1")
            hex_bits = row["hex_msb"]
            if len(hex_bits) != ((bits + 7) // 8) * 2 or not re.fullmatch(r"[0-9A-F]+", hex_bits):
                raise ValueError("Packed bit count and hex payload length disagree")
            raw = bytes.fromhex(hex_bits)
            unused = (-bits) % 8
            if unused and (raw[-1] & ((1 << unused) - 1)):
                raise ValueError("Nonzero unused low bits in last byte")
            blocks.append(BitBlock(index, epoch, bits, first, last,
                                   row["discontinuity_before"] == "1",
                                   row["window_end"] == "1", hex_bits))
            if len(blocks) > capacity:
                raise ValueError("More blocks than declared capacity")
    if expected != len(blocks):
        raise ValueError("CSV block count differs from capture metadata")
    if sum(b.bit_count for b in blocks) != total_bits:
        raise ValueError("CSV recorded_bits does not match data")
    return metadata, blocks


def segments(blocks: list[BitBlock]) -> list[dict]:
    """Build loss-aware contiguous runs; NOT RF packet framing.

    Never join across epochs, explicit window ends, or lost-edge markers.
    A segment can still start/end halfway through an on-air transmission.
    """
    result: list[dict] = []
    bits = ""
    epoch = 0
    start = end = 0
    previous: BitBlock | None = None

    def emit() -> None:
        if not bits:
            return
        extra = (-len(bits)) % 8
        padded = bits + "0" * extra
        raw = bytes(int(padded[i:i + 8], 2) for i in range(0, len(padded), 8))
        result.append({"epoch": epoch, "first_us": start, "last_us": end,
                       "bit_count": len(bits), "hex_msb": raw.hex().upper(),
                       "protocol_validated": False})

    for b in blocks:
        boundary = previous is not None and (
            previous.epoch != b.epoch or previous.window_end or b.discontinuity_before)
        if boundary:
            emit()
            bits = ""
        if not bits:
            epoch = b.epoch
            start = b.first_us
        # A clock regression within an apparently continuous run is not safe
        # to treat as a valid stream; isolate it as a new segment.
        elif b.first_us < end:
            emit()
            bits = ""
            epoch = b.epoch
            start = b.first_us
        bits += b.bitstring()
        end = b.last_us
        previous = b
    emit()
    return result


def summarize(meta: dict[str, str], blocks: list[BitBlock], runs: list[dict]) -> dict:
    return {
        "schema": "rflink-sync-bits-inspection-v1",
        "input_schema": meta["schema"],
        "generation": int(meta["generation"]),
        "blocks": len(blocks),
        "recorded_bits": sum(b.bit_count for b in blocks),
        "epochs": sorted({b.epoch for b in blocks}),
        "discontinuity_markers": sum(b.discontinuity_before for b in blocks),
        "window_end_markers": sum(b.window_end for b in blocks),
        "segments": len(runs),
        "max_segment_bits": max((s["bit_count"] for s in runs), default=0),
        "capacity_reached": meta["full"] == "1",
        "rf_profile_validated": False,
        "itho_messages_validated": 0,
        "ramses_messages_validated": 0,
        "note": "Capture structure only; decoder and on-air compatibility are not yet tested.",
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv", type=Path, help="private RFLink bench bit capture CSV")
    parser.add_argument("--segments-json", type=Path,
                        help="optional PRIVATE bit-packed segments for offline replay")
    args = parser.parse_args()
    try:
        meta, blocks = read_capture(args.csv)
        runs = segments(blocks)
        result = summarize(meta, blocks, runs)
        if args.segments_json:
            payload = {"schema": "rflink-sync-bits-replay-v1",
                       "source_generation": int(meta["generation"]),
                       "segments": runs}
            # A private local file, never a GitHub artifact.
            with args.segments_json.open("x", encoding="utf-8") as output:
                try:
                    args.segments_json.chmod(0o600)
                except OSError:
                    pass  # Windows ACLs are managed by the host, not chmod.
                json.dump(payload, output, indent=2)
                output.write("\n")
        print(json.dumps(result, indent=2))
        return 0
    except (OSError, ValueError, csv.Error) as exc:
        print(f"Invalid RFLink capture: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
