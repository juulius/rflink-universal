#!/usr/bin/env python3
"""Safely stage a one-time RFLink source ZIP; never execute imported code."""
from __future__ import annotations
import argparse
from pathlib import Path, PurePosixPath
from zipfile import ZipFile

ALLOWED = ('firmware/', 'rflink_builder/', 'releases/', 'tests_repo/', 'tests_previous/')
EXACT = {'repository.yaml'}
REQUIRED = {'repository.yaml', 'rflink_builder/config.yaml', 'rflink_builder/Dockerfile',
            'rflink_builder/server.py', 'firmware/CMakeLists.txt', 'firmware/partitions.csv',
            'firmware/boards/rflink_s3_n16r8.json'}
DENIED_NAMES = {'secrets.yaml', 'secrets.json', 'mqtt_settings.json', 'local_settings.json',
                'board_profile.local.json', 'credentials.json', '.git', '.github', '.ssh',
                '__pycache__', '.pytest_cache', 'id_rsa', 'id_ed25519'}
DENIED_SUFFIXES = ('.pem', '.key', '.p12', '.pfx', '.pyc', '.pyo')


def stage(archive: Path, destination: Path):
    if not archive.is_file() or archive.stat().st_size > 16_000_000:
        raise ValueError('Missing source ZIP or size exceeds 16 MB')
    names, size = set(), 0
    with ZipFile(archive) as zf:
        entries = zf.infolist()
        if len(entries) > 800 or zf.testzip() is not None:
            raise ValueError('Excessive ZIP entries or corrupt content')
        for item in entries:
            if item.is_dir():
                continue
            raw = item.filename
            parts = raw.split('/')
            path = PurePosixPath(raw)
            if (not raw or raw.startswith('/') or '\\' in raw or '\x00' in raw
                or ':' in raw or len(raw) > 240 or any(part in ('', '.', '..') for part in parts)):
                raise ValueError('Unsafe ZIP path: ' + raw[:120])
            if raw not in EXACT and not raw.startswith(ALLOWED):
                raise ValueError('Outside approved source folders: ' + raw)
            if any(part.lower() in DENIED_NAMES or part.lower().startswith('.env')
                   for part in parts) or raw.lower().endswith(DENIED_SUFFIXES):
                raise ValueError('Personal or generated file: ' + raw)
            if ((item.external_attr >> 16) & 0o170000) == 0o120000:
                raise ValueError('Symbolic link: ' + raw)
            if raw in names:
                raise ValueError('Duplicate ZIP path: ' + raw)
            names.add(raw)
            size += item.file_size
            if item.file_size > 8_000_000 or size > 40_000_000:
                raise ValueError('Expanded source exceeds safe size limits')
            target = destination / path
            target.parent.mkdir(parents=True, exist_ok=True)
            with zf.open(item) as source, target.open('wb') as sink:
                while block := source.read(65536):
                    sink.write(block)
    missing = REQUIRED - names
    if missing:
        raise ValueError('Incomplete RFLink source ZIP: ' + ', '.join(sorted(missing)))
    return len(names), size


if __name__ == '__main__':
    p = argparse.ArgumentParser(description='Validate and stage RFLink source ZIP')
    p.add_argument('archive', type=Path)
    p.add_argument('destination', type=Path)
    args = p.parse_args()
    count, expanded = stage(args.archive, args.destination)
    print(f'Checked and staged {count} files ({expanded} bytes); review branch required')
