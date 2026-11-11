#!/usr/bin/env python3
from pathlib import Path
import shutil, subprocess, sys, tempfile
ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / 'rflink_universal' / 'components' / 'rflink_universal'
PATCH = ROOT / 'rflink_universal' / 'maintenance' / 'legacy_hotfix' / 'apply_hotfix.py'
with tempfile.TemporaryDirectory() as temp:
    folder = Path(temp) / 'rflink_universal' / 'components' / 'rflink_universal'
    folder.mkdir(parents=True)
    original = (SRC / 'rflink_universal.cpp').read_text()
    marker = '// RFLINK_WEBHANDLER_LINK_HOTFIX_20260918'
    # Simulate RC4 layout: a separate webhandler CPP, and unmodified other code.
    core, impl = original.split(marker, 1)
    web = impl.split('#include "rflink_web.h"', 1)[1]
    (folder / 'rflink_universal.cpp').write_text(core)
    (folder / 'rflink_web.cpp').write_text('#include "rflink_web.h"' + web)
    for attempt in range(2):
        result = subprocess.run([sys.executable, str(PATCH), str(folder.parents[1])], capture_output=True, text=True)
        assert result.returncode == 0, result.stdout + result.stderr
    assert marker in (folder / 'rflink_universal.cpp').read_text()
    assert not (folder / 'rflink_web.cpp').exists()
    assert len(list(folder.glob('rflink_universal.cpp.backup_*'))) == 1
    assert len(list(folder.glob('rflink_web.cpp.backup_*'))) == 1
print('RC4 in-place hotfix test + idempotence: PASS')
