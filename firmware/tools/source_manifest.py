#!/usr/bin/env python3
"""Deterministic source SHA-256 manifest for RFLink native releases."""
import hashlib
import json
from pathlib import Path
from generate_board_header import load as load_board, board_hash
ROOT = Path(__file__).resolve().parents[1]
SKIP_DIRS = {'build','release','__pycache__','.git','.pytest_cache','managed_components','.idf_component_manager','portable_sdk'}
# IDF creates these at project root; they are build *outputs* and must not
# invalidate the source snapshot taken before idf.py build.
SKIP_ROOT_FILES = {'sdkconfig','sdkconfig.old','dependencies.lock'}

def snapshot():
    paths = sorted((f for f in ROOT.rglob('*') if f.is_file() and not any(part in SKIP_DIRS for part in f.relative_to(ROOT).parts) and f.name != 'SOURCE_MANIFEST.json' and not (len(f.relative_to(ROOT).parts)==1 and f.name in SKIP_ROOT_FILES)), key=lambda p: p.relative_to(ROOT).as_posix())
    data=[]
    for f in paths:
        raw=f.read_bytes()
        data.append({'path':f.relative_to(ROOT).as_posix(), 'bytes':len(raw), 'sha256':hashlib.sha256(raw).hexdigest()})
    joined=''.join(f"{x['path']}\0{x['sha256']}\n" for x in data).encode()
    return {'version':'0.7.0-n16r8-s20-b'+board_hash(load_board()), 'codename':'Radioactive',
            'status':'source-only-no-ESP-IDF-build',
            'source_fingerprint':hashlib.sha256(joined).hexdigest()[:12],
            'files':data}

def verify():
    saved=json.loads((ROOT/'SOURCE_MANIFEST.json').read_text())
    active=snapshot()
    if saved!=active:
        raise ValueError('Source manifest does not match the current source files')
    return active

if __name__=='__main__':
    import sys
    try:
        if len(sys.argv)>1 and sys.argv[1]=='--verify':
            result=verify()
        else:
            result=snapshot()
            (ROOT/'SOURCE_MANIFEST.json').write_text(json.dumps(result,indent=2)+'\n')
        print(f"SOURCE MANIFEST {result['version']} fingerprint={result['source_fingerprint']} ({len(result['files'])} files)")
    except (ValueError,OSError,FileNotFoundError) as e:
        print('FAIL manifest:',e,file=sys.stderr);sys.exit(1)
