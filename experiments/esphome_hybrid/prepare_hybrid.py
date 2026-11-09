#!/usr/bin/env python3
"""Prepare an isolated ESPHome/native-decoder proof from the working v0.6.6 ZIP.

Never edits the supplied ZIP, original ESPHome directory, current Builder or
native firmware. Does not copy any secrets.yaml. Source code is taken from
this repository at the commit where this script runs, not the network.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import stat
from pathlib import Path, PurePosixPath
from zipfile import ZipFile, BadZipFile, ZIP_DEFLATED

REPO = Path(__file__).resolve().parents[2]
EXPERIMENT = Path(__file__).resolve().parent
ZIP_ROOT = "RFLink_Universal_v0.6.6_RadioGaga_RELEASE_HASH_FIX_SOURCE"
PROBE_DIR = "rflink_universal/hybrid_probe"
YAML_NAME = "rflink-universal.yaml"
ESP_HOME = "esphome:\n  name: ${name}\n  friendly_name: ${friendly_name}\n"
LEGACY_CALLBACK = (
    "            id(rflink_core).receive_packet(2, x, rssi, lqi, freq_offset);"
)
INCLUDE_BLOCK = (
    "  # Hybrid SPI/RMT ownership remains entirely in ESPHome.\n"
    "  # Diagnostic decoder only; header is included and directory is compiled.\n"
    "  includes:\n"
    "    - rflink_universal/hybrid_probe/hybrid_probe.hpp\n"
    "    - rflink_universal/hybrid_probe/\n"
)
HYBRID_CALLBACK = (
    LEGACY_CALLBACK + "\n"
    "            // Shadow-decode the SAME 63-byte packet in the v0.7 C++ core.\n"
    "            // Legacy v0.6.6 decoding, devices, TX and pairing are unchanged.\n"
    "            rflink_hybrid::observe_868_packet(x);\n"
    "            static uint32_t hybrid_seen = 0;\n"
    "            if (++hybrid_seen <= 3 || hybrid_seen % 64 == 0) {\n"
    "              const auto d = rflink_hybrid::counters();\n"
    '              ESP_LOGI("rflink.hybrid", "shadow 868: seen=%lu native_in=%lu itho=%lu ramses=%lu unknown=%lu rejected=%lu overlap=%lu",\n'
    "                (unsigned long)hybrid_seen, (unsigned long)d.packet_inputs,\n"
    "                (unsigned long)d.itho_messages, (unsigned long)d.ramses_messages,\n"
    "                (unsigned long)d.unknown_packet_inputs, (unsigned long)d.rejected_inputs,\n"
    "                (unsigned long)d.overlapping_decodes);\n"
    "            }"
)
# Pure C++ only: no native CC1101, SPI, RMT, UART, OTA or web source is copied.
SOURCES = {
    "hybrid_probe.hpp": "experiments/esphome_hybrid/hybrid_probe.hpp",
    "hybrid_probe.cpp": "experiments/esphome_hybrid/hybrid_probe.cpp",
    "rflink_native_decoder.hpp": "firmware/components/rflink_decoder_core/include/rflink_native_decoder.hpp",
    "rflink_native_decoder.cpp": "firmware/components/rflink_decoder_core/src/rflink_native_decoder.cpp",
    "rflink_itho_codec.hpp": "firmware/components/rflink_decoder_core/include/rflink_itho_codec.hpp",
    "rflink_itho_codec.cpp": "firmware/components/rflink_decoder_core/src/rflink_itho_codec.cpp",
    "rflink_ramses_codec.hpp": "firmware/components/rflink_decoder_core/include/rflink_ramses_codec.hpp",
    "rflink_ramses_codec.cpp": "firmware/components/rflink_decoder_core/src/rflink_ramses_codec.cpp",
    "rflink_rx_sink.hpp": "firmware/components/rflink_radio_hal/include/rflink_rx_sink.hpp",
    "rflink_radio_hal.hpp": "firmware/components/rflink_radio_hal/include/rflink_radio_hal.hpp",
    "rflink_board_generated.hpp": "firmware/components/rflink_radio_hal/include/rflink_board_generated.hpp",
}


def sha256(content: bytes) -> str:
    return hashlib.sha256(content).hexdigest()


def patch_legacy_yaml(yaml: str) -> str:
    """Two strict insertions. Fail closed if source layout or hardware drifts."""
    if yaml.count(ESP_HOME) != 1 or yaml.count(LEGACY_CALLBACK) != 1:
        raise ValueError("Unexpected v0.6.6 YAML; refusing to patch hardware or callbacks")
    if "  includes:" in yaml or "rflink_hybrid::" in yaml:
        raise ValueError("Source already has custom includes/a hybrid adapter")
    for required in ("type: esp-idf", "rmt_symbols: 96", "433.42MHz",
                     "433.92MHz", "868.2999MHz", "remote_receiver:"):
        if required not in yaml:
            raise ValueError("Working v0.6.6 radio baseline changed: " + required)
    return yaml.replace(ESP_HOME, ESP_HOME + INCLUDE_BLOCK, 1).replace(
        LEGACY_CALLBACK, HYBRID_CALLBACK, 1)


def read_source_zip(source: Path) -> dict[str, bytes]:
    """Read only the baseline YAML and rflink_universal source tree.

    Detect path traversal, symbolic links, duplicate entries, other versions,
    oversize archives and actual secrets. Do not extract arbitrary ZIP paths.
    """
    selected: dict[str, bytes] = {}
    size = 0
    with ZipFile(source) as archive:
        if len(archive.infolist()) > 3500:
            raise ValueError("Source ZIP contains too many files")
        for info in archive.infolist():
            name = PurePosixPath(info.filename)
            if name.is_absolute() or ".." in name.parts or name.parts[0] != ZIP_ROOT:
                raise ValueError("Unexpected or unsafe source ZIP entry: " + info.filename)
            mode = info.external_attr >> 16
            if stat.S_ISLNK(mode):
                raise ValueError("Source ZIP must not contain symlinks")
            if info.is_dir():
                continue
            rel = PurePosixPath(*name.parts[1:])
            if rel.as_posix() != YAML_NAME and (
                not rel.parts or rel.parts[0] != "rflink_universal"):
                continue
            if rel.name.lower() in {"secrets.yaml", ".env", "credentials.json"}:
                continue
            if "__pycache__" in rel.parts or rel.suffix in {".pyc", ".pyo"}:
                continue
            if rel.as_posix() in selected:
                raise ValueError("Duplicate source entry: " + str(rel))
            size += info.file_size
            if size > 50 * 1024 * 1024:
                raise ValueError("Unreasonably large source archive")
            selected[rel.as_posix()] = archive.read(info)
    if YAML_NAME not in selected or not any(
        p.startswith("rflink_universal/components/rflink_universal/")
        for p in selected
    ):
        raise ValueError("Not a complete Radio Gaga v0.6.6 ESPHome source archive")
    return selected


def prepare(source_zip: Path, output: Path) -> Path:
    source_zip = source_zip.resolve()
    output = output.resolve()
    if output.exists():
        raise FileExistsError("Output directory already exists; never overwrite working v0.6.6")
    source_files = read_source_zip(source_zip)
    yaml = patch_legacy_yaml(source_files[YAML_NAME].decode("utf-8"))
    source_files[YAML_NAME] = yaml.encode("utf-8")
    # Validate every native path before touching the output directory.
    native = {name: (REPO / path).read_bytes() for name, path in SOURCES.items()}
    # The working v0.6.6 build already links the RAMSES-II codec under the
    # global rflink_ramses_ii namespace. Both versions export Frame::Frame(),
    # decodePacked(), etc. Linking both without isolation causes duplicate
    # symbols even though the host-only adapter smoke test succeeds.
    # Rename ONLY the copied experimental native decoder and RAMSES codec,
    # leaving the legacy firmware and native v0.7 repository source intact.
    rename_files = (
        "rflink_native_decoder.cpp", "rflink_ramses_codec.hpp",
        "rflink_ramses_codec.cpp",
    )
    old_namespace = b"rflink_ramses_ii"
    new_namespace = b"rflink_hybrid_ramses_ii"
    for filename in rename_files:
        if old_namespace not in native[filename]:
            raise ValueError("Native RAMSES namespace contract changed: " + filename)
        native[filename] = native[filename].replace(old_namespace, new_namespace)
    # Fail if a future refactor adds a reference which would collide with the
    # independent RAMSES-II decoder still compiled by v0.6.6 ESPHome.
    if any(old_namespace in data for data in native.values()):
        raise ValueError("Unisolated native RAMSES symbol in experiment sources")
    output.mkdir(parents=True)
    try:
        for rel, data in source_files.items():
            target = output / rel
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
        for filename, data in native.items():
            target = output / PROBE_DIR / filename
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
        manifest = {
            "mode": "read-only ESPHome 0.6.6 plus native 0.7 868 decoder",
            "source_zip_sha256": sha256(source_zip.read_bytes()),
            "radio_owner": "ESPHome v0.6.6 (unchanged)",
            "native_files": {n: sha256(data) for n, data in sorted(native.items())},
            "flashed": False,
            "hardware_tested": False,
        }
        (output / "HYBRID_PROBE_MANIFEST.json").write_text(
            json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    except BaseException:
        shutil.rmtree(output, ignore_errors=True)
        raise
    return output / YAML_NAME


def archive_source_tree(source_dir: Path, archive_path: Path) -> None:
    """Create a reproducible, standalone source ZIP without secrets.

    This is a source-only artifact, NEVER a flashable ESP32 binary. Refuse
    overwrites, symlinks, or a ZIP nested inside the source directory.
    """
    source_dir = source_dir.resolve()
    archive_path = archive_path.resolve()
    if archive_path.exists():
        raise FileExistsError("Source archive already exists; refusing overwrite")
    if archive_path == source_dir or source_dir in archive_path.parents:
        raise ValueError("Output source archive must be outside the build tree")
    archive_path.parent.mkdir(parents=True, exist_ok=True)
    with ZipFile(archive_path, "x", compression=ZIP_DEFLATED,
                 compresslevel=9) as archive:
        for file in sorted(source_dir.rglob("*")):
            if file.is_symlink():
                raise ValueError("Source tree must not contain symlinks")
            if not file.is_file():
                continue
            rel = file.relative_to(source_dir).as_posix()
            if file.name.lower() in {"secrets.yaml", ".env", "credentials.json"}:
                raise ValueError("Secrets found in source tree: " + rel)
            archive.write(file, rel)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True,
                        help="Local working v0.6.6 Radio Gaga source ZIP")
    parser.add_argument("--output", type=Path, required=True,
                        help="NEW isolated experiment directory (must not exist)")
    parser.add_argument("--archive", type=Path, default=None,
                        help="Optionally write a NEW standalone source-only ZIP")
    args = parser.parse_args()
    try:
        yaml = prepare(args.source, args.output)
        if args.archive is not None:
            archive_source_tree(args.output, args.archive)
    except (OSError, ValueError, BadZipFile) as exc:
        parser.exit(2, f"HYBRID PREPARE REFUSED: {exc}\n")
    print("Prepared isolated read-only hybrid experiment:", yaml)
    if args.archive is not None:
        print("Source-only ZIP created:", args.archive.resolve())
    print("No firmware was compiled, flashed or deployed; no Wi-Fi/OTA secrets copied.")


if __name__ == "__main__":
    main()
