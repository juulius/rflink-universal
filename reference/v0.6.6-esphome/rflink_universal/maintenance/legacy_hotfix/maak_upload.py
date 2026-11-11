#!/usr/bin/env python3
"""Safe one-click source packaging for RFLink (standard library only).

Writes a complete project upload ZIP and tar.gz; never edits source files.
Does not compile firmware and does not upload anything to the internet.
"""
from __future__ import annotations

import argparse
import hashlib
import io
from pathlib import Path
import sys
import tarfile
import zipfile

VERSION = "v0.3.3-RC4-HF4-USERLIB2-PROFILES1-UI4-SOMFY1-ITHO1"
ARCHIVE_STEM = f"RFLink_Universal_{VERSION}_UPLOAD"
REQUIRED = (
    "rflink-universal.yaml",
    "rflink_universal/components/rflink_universal/__init__.py",
    "rflink_universal/components/rflink_universal/rflink_universal.cpp",
    "rflink_universal/components/rflink_universal/web_ui.h",
    "rflink_universal/decoders/itho_rft/manifest.json",
    "rflink_universal/ui/index.html",
    "rflink_universal/options/README_RGB.md",
    "rflink_universal/components/rflink_universal/status_led.h",
    "rflink_universal/tests/test_led_status.cpp",
    "rflink_universal/tools/maak_upload.py",
    "rflink_universal/tools/generate_profiles.py",
    "rflink_universal/profiles/acurite_609txc.json",
    "rflink_universal/components/rflink_universal/profile_catalog_generated.h",
    "USER_DECODER_GUIDE.md",
)
TOP_FILES = (
    "rflink-universal.yaml", "MAAK_UPLOAD.bat", "MAAK_RELEASE.bat", "UPDATE_RFLINK.bat",
    "TEST_UPDATE_RELEASE.bat", "START_HERE.md", "README.md", "RELEASE_NOTES.md",
    "TEST_RESULTS.txt", "LOGGING_RUNTIME.md", "CLEANUP_REPORT.md", "cleanup_manifest.json", "USER_DECODER_GUIDE.md", "INSTELLINGEN.md", "INSTALLEREN_EN_UPDATEN.md",
    "LIBRARY_AND_PROFILES.md", "RADIO_PROFILES.md", "UX_PRINCIPLES.md", "DECODER_API_V2.md", "DECODER_UX1.md",
    "DECODER_EXPANSION.md", "TX1_BEDIENING.md", "RADIOLOOS_TESTEN.md", "BOOTFIX_README.md",
    "LEDFIX_README.md", "CONTRIBUTING.md", "SECURITY.md", "PUBLICATION_CHECKLIST.md",
)
EXCLUDED_DIRS = {"dist", "build", ".esphome", ".git", "__pycache__", "venv", ".venv", "node_modules", ".pio", "logs", "backup", "backups"}
EXCLUDED_FILES = {"secrets.yaml", ".env", ".env.local", "credentials.json", "credentials.yaml", "passwords.txt", "id_rsa", "id_ed25519"}
EXCLUDED_SUFFIXES = {".zip", ".gz", ".log", ".bin", ".elf", ".o", ".obj", ".pyc", ".pem", ".key", ".p12", ".pfx", ".db", ".sqlite", ".sqlite3"}


def wanted(rel: Path) -> bool:
    parts = rel.parts
    if any(p.lower() in EXCLUDED_DIRS or p.startswith(".") for p in parts[:-1]):
        return False
    filename = parts[-1].lower()
    if filename in EXCLUDED_FILES or filename.startswith("secrets.") and filename != "secrets.example.yaml":
        return False
    if filename.startswith(".") or ".backup_" in filename or filename.endswith(("~", ".bak", ".tmp", ".swp")):
        return False
    if rel.suffix.lower() in EXCLUDED_SUFFIXES:
        return False
    return True


def collect(root: Path) -> dict[str, bytes]:
    for name in REQUIRED:
        if not (root / name).is_file():
            raise ValueError(f"VERPLICHT bestand ontbreekt: {name}")
    files: dict[str, bytes] = {}
    for name in TOP_FILES:
        file = root / name
        if not file.is_file():
            raise ValueError(f"Verplicht hoofdbestand ontbreekt: {name}")
        files[name] = file.read_bytes()
    for file in sorted((root / "docs").rglob("*")):
        if file.is_symlink():
            raise ValueError(f"Symbolische link niet toegelaten: {file}")
        if file.is_file():
            rel = file.relative_to(root)
            if wanted(rel):
                files[rel.as_posix()] = file.read_bytes()
    source = root / "rflink_universal"
    for file in sorted(source.rglob("*")):
        if file.is_symlink():
            raise ValueError(f"Symbolische link niet toegelaten: {file}")
        if not file.is_file():
            continue
        rel = file.relative_to(root)
        if wanted(rel):
            files[rel.as_posix()] = file.read_bytes()
    if len(files) < 35:
        raise ValueError(f"Onverwacht weinig projectbestanden: {len(files)} (pakket gestopt)")
    if "// RFLINK_WEBHANDLER_LINK_HOTFIX_20260918" not in files[
        "rflink_universal/components/rflink_universal/rflink_universal.cpp"
    ].decode("utf-8"):
        raise ValueError("WebHandler-linkerfix ontbreekt in hoofdbron: stop")
    if any(k.lower().endswith("/secrets.yaml") or k.lower() == "secrets.yaml" for k in files):
        raise ValueError("secrets.yaml mag nooit in het pakket")
    return files


def verify_zip(path: Path, expected: dict[str, bytes]) -> None:
    with zipfile.ZipFile(path, "r") as archive:
        if archive.testzip() is not None:
            raise ValueError("ZIP CRC controle mislukt")
        if sorted(archive.namelist()) != sorted(expected):
            raise ValueError("ZIP bestandslijst wijkt af")
        for name, content in expected.items():
            if archive.read(name) != content:
                raise ValueError(f"ZIP inhoud is anders: {name}")


def verify_tar(path: Path, expected: dict[str, bytes]) -> None:
    with tarfile.open(path, "r:gz") as archive:
        if sorted(archive.getnames()) != sorted(expected):
            raise ValueError("tar.gz bestandslijst wijkt af")
        for name, content in expected.items():
            member = archive.getmember(name)
            if not member.isfile():
                raise ValueError(f"Ongeldig tar item: {name}")
            file = archive.extractfile(member)
            if file is None or file.read() != content:
                raise ValueError(f"tar.gz inhoud is anders: {name}")


def create(root: Path, output_dir: Path) -> tuple[Path, Path, int]:
    files = collect(root)
    output_dir.mkdir(parents=True, exist_ok=True)
    zip_path = output_dir / (ARCHIVE_STEM + ".zip")
    tar_path = output_dir / (ARCHIVE_STEM + ".tar.gz")
    zip_temp = output_dir / (ARCHIVE_STEM + ".zip.tmp")
    tar_temp = output_dir / (ARCHIVE_STEM + ".tar.gz.tmp")
    try:
        with zipfile.ZipFile(zip_temp, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=8) as archive:
            for name, content in files.items():
                archive.writestr(name, content)
        verify_zip(zip_temp, files)
        with tarfile.open(tar_temp, "w:gz") as archive:
            for name, content in files.items():
                info = tarfile.TarInfo(name)
                info.size = len(content)
                info.mode = 0o644
                info.mtime = 0
                archive.addfile(info, io.BytesIO(content))
        verify_tar(tar_temp, files)
        zip_temp.replace(zip_path)
        tar_temp.replace(tar_path)
    finally:
        zip_temp.unlink(missing_ok=True)
        tar_temp.unlink(missing_ok=True)
    return zip_path, tar_path, len(files)


def main() -> int:
    parser = argparse.ArgumentParser(description="Maak gecontroleerde RFLink ZIP en tar.gz zonder secrets.yaml")
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2], help=argparse.SUPPRESS)
    parser.add_argument("--output-dir", type=Path, help=argparse.SUPPRESS)
    args = parser.parse_args()
    root = args.root.expanduser().resolve()
    output = args.output_dir.expanduser().resolve() if args.output_dir else root / "dist"
    try:
        zip_path, tar_path, count = create(root, output)
    except (OSError, ValueError, zipfile.BadZipFile, tarfile.TarError) as err:
        print(f"FOUT: geen gevalideerd pakket geproduceerd: {err}", file=sys.stderr)
        return 1
    print(f"OK: {count} bestanden gecontroleerd en verpakt. Broncode niet gewijzigd.")
    for path in (zip_path, tar_path):
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        print(f"BESTAND: {path}\nSHA256: {digest}")
    print("NIET automatisch geupload. Controleer zelf of broncode/documentatie persoonlijke waarden bevat.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
