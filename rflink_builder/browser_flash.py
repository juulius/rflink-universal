"""Validated, per-device ESP32-S3 factory image for browser Web Serial.

The first-flash image includes the private Wi-Fi/OTA NVS partition.
Only an authenticated same-origin Builder request may obtain the bytes;
never publish these images as GitHub artifacts or serve them to a CDN.
"""
from __future__ import annotations

import hashlib
from pathlib import Path
import subprocess
import sys
import tempfile

MAX_IMAGE = 16 * 1024 * 1024
EXPECTED = (
    ("bootloader", 0x00000),
    ("partition_table", 0x08000),
    ("provisioning_nvs", 0x09000),
    ("ota_data_initial", 0x19000),
    ("app_ota", 0x20000),
)


def verified_parts(manifest: dict, release: Path) -> list[tuple[int, Path]]:
    """Reject mismatched chip, layout, missing private NVS and invalid hashes."""
    if (manifest.get("test_only") is not True or manifest.get("chip") != "esp32s3"
        or manifest.get("flash_size_bytes") != MAX_IMAGE
        or manifest.get("bootloader_offset") != 0
        or manifest.get("partition_table_offset") != 0x8000
        or manifest.get("nvs_offset") != 0x9000
        or manifest.get("nvs_size_bytes") != 0x10000
        or manifest.get("otadata_offset") != 0x19000
        or manifest.get("ota_0_offset") != 0x20000
        or manifest.get("ota_slot_size_bytes") != 0x400000
        or not manifest.get("builder_device_id")):
        raise ValueError("Browser USB requires a validated 16MB ESP32-S3 first-install build with device provisioning")
    files = manifest.get("files")
    if not isinstance(files, dict):
        raise ValueError("Missing validated flash file manifest")
    release = Path(release)
    entries: list[tuple[int, Path]] = []
    for key, offset in EXPECTED:
        meta = files.get(key)
        if not isinstance(meta, dict):
            raise ValueError("Incomplete first-install image: " + key)
        filename = meta.get("filename")
        if not isinstance(filename, str) or filename in ("", ".", "..") or Path(filename).name != filename:
            raise ValueError("Unsafe image filename: " + key)
        file = release / filename
        if not file.is_file() or file.is_symlink():
            raise ValueError("Missing or unsafe firmware image: " + key)
        data = file.read_bytes()
        expected_size = meta.get("bytes", meta.get("size"))
        if (not isinstance(expected_size, int) or isinstance(expected_size, bool)
            or len(data) != expected_size or not data):
            raise ValueError("Image size mismatch: " + key)
        if hashlib.sha256(data).hexdigest() != meta.get("sha256"):
            raise ValueError("Image SHA-256 mismatch: " + key)
        if offset + len(data) > MAX_IMAGE:
            raise ValueError("Image exceeds 16MB flash capacity")
        if key == "provisioning_nvs" and (len(data) != 0x10000 or meta.get("offset") != 0x9000):
            raise ValueError("Missing correct per-device NVS provisioning partition")
        if key == "app_ota" and (data[0] != 0xE9 or len(data) >= 0x400000):
            raise ValueError("Invalid ESP32-S3 OTA application image")
        if key == "bootloader" and data[0] != 0xE9:
            raise ValueError("Invalid ESP32-S3 bootloader image")
        entries.append((offset, file))
    for i, (start, file) in enumerate(entries[:-1]):
        next_offset = entries[i + 1][0]
        if start + file.stat().st_size > next_offset:
            raise ValueError("Firmware partitions overlap: " + file.name)
    return entries


def browser_factory_image(manifest: dict, release: Path, env: dict | None = None) -> bytes:
    """Merge verified IDF images with Espressif esptool; never flash the HA host."""
    entries = verified_parts(manifest, release)
    with tempfile.TemporaryDirectory(prefix="rflink-browser-flash-") as tmp:
        output = Path(tmp) / "RFLink_ESP32S3_private_factory.bin"
        command = [sys.executable, "-m", "esptool", "--chip", "esp32s3", "merge-bin",
                   "--flash-size", "16MB", "-o", str(output)]
        for offset, file in entries:
            command.extend((hex(offset), str(file)))
        try:
            result = subprocess.run(command, cwd=release, env=env, capture_output=True,
                                    timeout=120, check=False)
        except (OSError, subprocess.TimeoutExpired) as exc:
            raise RuntimeError("Could not prepare browser USB image using the installed ESP-IDF toolchain") from exc
        if result.returncode != 0 or not output.is_file():
            # Tool stdout/stderr could contain local paths: do not expose them to the browser.
            raise RuntimeError("Espressif esptool could not merge the validated first-install partitions")
        size = output.stat().st_size
        if not 0x20000 < size <= MAX_IMAGE:
            raise RuntimeError("Merged factory image has an unexpected size")
        data = output.read_bytes()
        if data[0] != 0xE9 or data[0x8000:0x8002] == b"\\xff\\xff":
            raise RuntimeError("Merged factory image has an invalid bootloader or partition table")
        return data
