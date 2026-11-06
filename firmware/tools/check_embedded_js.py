#!/usr/bin/env python3
"""Optional Node.js syntax test for inline JavaScript in the native test website."""
import re
import shutil
import subprocess
import tempfile
from pathlib import Path
root=Path(__file__).resolve().parents[1]
js=re.findall(r'<script>(.*?)</script>',(root/'main/rflink_web_ota.cpp').read_text(),re.S)
assert len(js)>=2,'Expected OTA upload and radio/status event scripts'
if shutil.which('node'):
    with tempfile.TemporaryDirectory(prefix='rflink-web-check-') as temp:
        for i,source in enumerate(js):
            file=Path(temp)/f'inline_{i}.js'
            file.write_text(source)
            subprocess.run(['node','--check',str(file)],check=True)
    print(f'PASS {len(js)} native website inline JavaScript syntax checks via Node.js')
else:
    print(f'SKIP {len(js)} JavaScript syntax checks (Node.js not installed)')
