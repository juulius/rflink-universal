#!/usr/bin/env python3
"""Validate all decoder packs and compare checked-in host catalog with manifests."""
import importlib.util
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DECODERS = ROOT / "decoders"
COMPONENT = ROOT / "components" / "rflink_universal"
DOC_DECODERS = ROOT.parent / "docs" / "source" / "rflink_universal" / "decoders"
spec = importlib.util.spec_from_file_location("rflink_manifest_schema", COMPONENT / "manifest_schema.py")
manifest_schema = importlib.util.module_from_spec(spec)
spec.loader.exec_module(manifest_schema)


def main():
    errors, packs = [], []
    for folder in sorted(p for p in DECODERS.iterdir() if p.is_dir() and not p.name.startswith('_')):
        path = folder / 'manifest.json'
        source = folder / 'decoder.cpp'
        if not (path.is_file() and source.is_file() and (DOC_DECODERS / folder.name / 'README.md').is_file()):
            errors.append(f'{folder.name}: decoder.cpp, manifest.json and docs/source/.../README.md required')
            continue
        try:
            m = manifest_schema.load_manifest(path)
            if m.get('enabled', True):
                packs.append((folder, m))
            tx_source = (folder / "tx_encoder.cpp")
            registered = re.findall(r"\bRFLINK_REGISTER_TX_ENCODER\(\s*([A-Za-z_]\w*)\s*\)",
                                    tx_source.read_text(encoding="utf-8") if tx_source.exists() else "")
            claimed = sum(cap["transmit"]["encoder"] != "none" for cap in m["capabilities"])
            if len(registered) != claimed:
                errors.append(f"{folder.name}: TX manifest claims {claimed} but {len(registered)} encoders registered")
            code = source.read_text(encoding='utf-8')
            if 'RFLINK_REGISTER_DECODER(' not in code:
                errors.append(f'{folder.name}: decoder is not registered')
            if '#include "esphome/components/rflink_universal/decoder_registry.h"' not in code:
                errors.append(f'{folder.name}: use the canonical decoder registry include')
            for cap in m['capabilities']:
                if not re.search(r'\b(?:result|r)\.protocol\s*=\s*"' + re.escape(cap['protocol']) + r'"', code):
                    errors.append(f'{folder.name}: protocol {cap["protocol"]} has no decoder.cpp protocol assignment')
                for field in cap['fields']:
                    # A dynamic key is permitted only for the schema-reviewed Dooya check nibbles.
                    marker = field['id'].replace('*', '') if field['id'] == 'check_button_*' else field['id']
                    if not re.search(r'\b(?:result|r)\.fields\.push_back\(\{\s*"' + re.escape(marker), code):
                        errors.append(f'{folder.name}: manifest field {field["id"]} not produced by decoder.cpp')
        except (ValueError, KeyError, TypeError) as exc:
            errors.append(str(exc))
    try:
        expected = manifest_schema.generate_header(packs)
        checked = (COMPONENT / 'capability_catalog_generated.h').read_text(encoding='utf-8')
        if checked != expected:
            errors.append('capability_catalog_generated.h outdated: regenerate with tools/generate_catalog.py')
    except (ValueError, FileNotFoundError) as exc:
        errors.append(str(exc))
    if errors:
        print('Decoder pack schema v2: FAIL')
        for err in errors: print(' -', err)
        return 1
    print(f'Decoder pack schema v2: PASS ({len(packs)} enabled packs, validated runtime catalog)')
    return 0


if __name__ == '__main__':
    sys.exit(main())
