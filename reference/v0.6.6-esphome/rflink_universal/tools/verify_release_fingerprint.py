#!/usr/bin/env python3
"""Developer verification only; Windows MAAK_RELEASE.bat needs no Python."""
import hashlib, re
from pathlib import Path
base = Path(__file__).resolve().parents[1]
identity = (base/'components/rflink_universal/firmware_identity.h').read_text()
components = [base/x for x in ('components','decoders','profiles','ui','options')]
files = sorted((p for directory in components for p in directory.rglob('*')
                if p.is_file() and p.suffix.lower() in ('.cpp','.h','.json','.html','.yaml','.py','.svg','.ico')
                and p.name != 'firmware_identity.h'), key=lambda p:p.relative_to(base).as_posix())
manifest = ''.join(f'{p.relative_to(base).as_posix()} {hashlib.sha256(p.read_bytes()).hexdigest()}\n' for p in files)
actual = hashlib.sha256(manifest.encode('utf-8')).hexdigest()[:12]
expected = re.search(r'RFLINK_FIRMWARE_BUILD_HASH\s*=\s*"([0-9a-f]{12})"',identity)
if not expected or expected.group(1) != actual:
    raise SystemExit('Release fingerprint mismatch: expected '+(expected.group(1) if expected else 'missing')+' actual '+actual+'. Bump release version, update hash and release notes before packing.')
print(f'Release fingerprint valid: {actual} across {len(files)} runtime files')
