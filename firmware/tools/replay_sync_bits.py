#!/usr/bin/env python3
"""Offline analysis of a PRIVATE RFLink 868-MHz synchronous DATA+CLOCK CSV.

The actual native RFLink Itho and RAMSES-II C++ decoders are compiled locally
with a host C++17 compiler. No hardware, networking, transmit, remote services,
device registration or upload. Capture bytes never leave the local computer.

Examples:
  python firmware/tools/replay_sync_bits.py RFLink_868_SyncBits_Bench.csv
  python firmware/tools/replay_sync_bits.py RFLink_Diagnostics_Full.csv
  python firmware/tools/replay_sync_bits.py capture.csv --try-inverted
  python firmware/tools/replay_sync_bits.py capture.csv --report-json private_report.json

IMPORTANT: RAMSES-II checksum-valid is stronger evidence than an Itho
structure-only candidate. The current native Itho decoder does NOT perform
full packet checksum verification. Neither proves on-air compatibility.
"""
from __future__ import annotations

import argparse
from collections import Counter
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

from inspect_sync_bits import read_capture, segments, summarize

FW = Path(__file__).resolve().parents[1]
CORE = FW / "components" / "rflink_decoder_core"
SCANNER = FW / "tools" / "sync_replay_native.cpp"
MAX_MATCHES = 5000
PROTOCOLS = {"itho", "ramses_ii"}
EVIDENCE = {"structure_only", "checksum_valid"}
POLARITY = {"normal", "inverted"}


def replay(csv_path: Path, try_inverted: bool = False) -> dict:
    metadata, blocks = read_capture(csv_path)
    runs = segments(blocks)  # Never bridge missing edges, TX epochs or window_end.
    overview = summarize(metadata, blocks, runs)
    compiler = shutil.which("g++") or shutil.which("clang++")
    if not compiler:
        raise ValueError("Host C++17 compiler not found (install g++ or clang++)")

    # Native C++ decoders operate on MSB-packed bits only. Bounded by the CSV
    # inspector's max 256 x 2048 bits and explicit metadata consistency checks.
    lines = [
        f"{idx} {run['bit_count']} {run['hex_msb']}"
        for idx, run in enumerate(runs)
    ]
    with tempfile.TemporaryDirectory(prefix="rflink-sync-replay-") as folder:
        exe = Path(folder) / "sync_replay_native"
        command = [
            compiler, "-std=c++17", "-O2", "-Wall", "-Wextra", "-Werror",
            "-I" + str(CORE / "include"),
            str(SCANNER),
            str(CORE / "src" / "rflink_itho_codec.cpp"),
            str(CORE / "src" / "rflink_ramses_codec.cpp"),
            "-o", str(exe),
        ]
        compiled = subprocess.run(command, capture_output=True, text=True,
                                  timeout=120, check=False)
        if compiled.returncode:
            raise ValueError("Local native decoder compilation failed: " +
                             compiled.stderr[-2500:])
        executed = subprocess.run(
            [str(exe)] + (["--try-inverted"] if try_inverted else []),
            input="\n".join(lines) + "\n", capture_output=True, text=True,
            timeout=120, check=False,
        )
        if executed.returncode:
            raise ValueError("Native replay rejected capture: " +
                             executed.stderr[-1200:])

    matches = []
    for line in executed.stdout.splitlines():
        cells = line.split("\t")
        if len(cells) != 7:
            raise ValueError("Unexpected native replay output format")
        seg, protocol, offset, span, evidence, detail, polarity = cells
        if protocol not in PROTOCOLS or evidence not in EVIDENCE or polarity not in POLARITY:
            raise ValueError("Unexpected native protocol classification")
        if (protocol == "itho" and evidence != "structure_only") or (
            protocol == "ramses_ii" and evidence != "checksum_valid"
        ):
            raise ValueError("Invalid native evidence-grade pairing")
        i, start, length = int(seg), int(offset), int(span)
        if i < 0 or i >= len(runs) or start < 0 or length <= 0 or (
            start + length > runs[i]["bit_count"]
        ):
            raise ValueError("Decoder match exceeds a loss-aware segment")
        if len(matches) >= MAX_MATCHES:
            raise ValueError("Too many decoder matches for a bounded bench report")
        # Do not include raw payload/addresses/remote IDs in ordinary reports.
        if not detail.startswith(("command=0x", "speed=", "timer_minutes=",
                                  "fan_percent=", "humidity_percent=", "type=")):
            raise ValueError("Unexpected native diagnostic field")
        matches.append({
            "segment": i,
            "epoch": runs[i]["epoch"],
            "bit_offset": start,
            "span_bits": length,
            "protocol": protocol,
            "evidence": evidence,
            "polarity": polarity,
            "summary": detail,
            "hardware_verified": False,
        })

    # Both protocol decoders may interpret different parts of ONE on-air
    # transmission. Native Itho speed fixtures include an embedded, checksum-
    # valid RAMSES-II logical message. Keep both interpretations, but never
    # count the nested RAMSES telegram as an independent RF/device event.
    itho_matches = [m for m in matches if m["protocol"] == "itho"]
    for m in matches:
        others = [other for other in matches
                  if other["protocol"] != m["protocol"] and
                  other["segment"] == m["segment"] and
                  other["polarity"] == m["polarity"] and
                  m["bit_offset"] < other["bit_offset"] + other["span_bits"] and
                  other["bit_offset"] < m["bit_offset"] + m["span_bits"]]
        m["overlapping_protocols"] = sorted({o["protocol"] for o in others})
        if m["protocol"] == "ramses_ii":
            m["embedded_in_itho_candidate"] = any(
                i["segment"] == m["segment"] and
                i["polarity"] == m["polarity"] and
                i["bit_offset"] <= m["bit_offset"] and
                m["bit_offset"] + m["span_bits"] <=
                i["bit_offset"] + i["span_bits"]
                for i in itho_matches
            )

    counts = Counter((m["protocol"], m["evidence"]) for m in matches)
    embedded = sum(
        m.get("embedded_in_itho_candidate", False) for m in matches
    )
    ramses_total = counts[("ramses_ii", "checksum_valid")]
    return {
        "schema": "rflink-sync-bits-native-replay-v1",
        "input_schema": overview["input_schema"],
        "source_generation": overview["generation"],
        "blocks": overview["blocks"],
        "segments": len(runs),
        "recorded_bits": overview["recorded_bits"],
        "discontinuity_markers": overview["discontinuity_markers"],
        "window_end_markers": overview["window_end_markers"],
        "candidate_itho_structure_only": counts[("itho", "structure_only")],
        "checksum_valid_ramses_ii": ramses_total,
        "ramses_ii_embedded_in_itho": embedded,
        "ramses_ii_outside_itho": ramses_total - embedded,
        "cross_decoder_overlap_matches": sum(
            bool(m["overlapping_protocols"]) for m in matches
        ),
        "matches": matches,
        "limitations": [
            "All results are offline decoder interpretations; no hardware verification.",
            "Itho native decoder is structural only; no full checksum validation.",
            "A checksum-valid RAMSES logical telegram may be embedded in an Itho RF capture; overlapping interpretations are not independent RF/device events.",
            "When an Itho header is corrupted, an embedded RAMSES telegram may still pass its own checksum; physical protocol attribution is unverified.",
            "Bit polarity and fixed Itho/RAMSES modem compatibility require real RF tests.",
            "No input segments are stitched across reported loss, RX epochs or capture windows.",
            "An unreported missed GPIO edge cannot be detected by this offline tool.",
        ],
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv", type=Path, help="private stand-alone 868 RAW capture or combined full diagnostics CSV")
    parser.add_argument("--try-inverted", action="store_true",
                        help="also search inverted polarity (diagnostic hypothesis only)")
    parser.add_argument("--report-json", type=Path,
                        help="create private bounded diagnostic JSON; never overwrite")
    args = parser.parse_args()
    try:
        result = replay(args.csv, args.try_inverted)
        if args.report_json:
            with args.report_json.open("x", encoding="utf-8") as output:
                try:
                    args.report_json.chmod(0o600)
                except OSError:
                    pass
                json.dump(result, output, indent=2)
                output.write("\n")
        print(json.dumps(result, indent=2))
        return 0
    except (OSError, ValueError, subprocess.TimeoutExpired) as exc:
        print(f"RFLink offline replay failed: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
