#!/usr/bin/env python3
"""Node syntax audit for the current native ESP-IDF web assets.

The old inline Advanced HTML page is retired: check the compiled native
webui/app.js instead of requiring obsolete inline script tags.
"""
import shutil
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parents[1]
page = (root / 'main/webui/index.html').read_text(encoding='utf-8')
script = root / 'main/webui/app.js'
assert 'src="/app.js"' in page, 'Native page must load the embedded app.js'
assert script.is_file(), 'Native JavaScript asset is missing'
assert '/rflink/api/' not in script.read_text(encoding='utf-8')
if shutil.which('node'):
    subprocess.run(['node', '--check', str(script)], check=True)
    print('PASS native embedded website JavaScript syntax via Node.js')
else:
    print('SKIP JavaScript syntax check (Node.js not installed)')
