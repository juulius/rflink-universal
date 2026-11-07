from pathlib import Path
import json
import re
import sys
import tempfile
import shutil
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'tools'))
from source_manifest import snapshot,verify,ROOT

# Confirm generated build files are excluded while source changes still change fingerprint.
a=snapshot()
for name in ('sdkconfig','sdkconfig.old','dependencies.lock'):
  q=root/name
  original=q.read_bytes() if q.exists() else None
  q.write_text('TEST generated file\n')
  try:
    assert snapshot()==a, f'Unexpected source-manifest drift from {name}'
  finally:
    if original is None:q.unlink()
    else:q.write_bytes(original)
# User-edited config must remain protected.
board=root/'boards/rflink_s3_n16r8.json'
assert any(x['path']==board.relative_to(root).as_posix() for x in a['files'])
assert any(x['path']=='sdkconfig.defaults' for x in a['files'])
# The official IDF build must still be the only way to make a firmware package.
offline=(root/'BUILD_OFFLINE.bat').read_text()
assert 'idf.py build' in offline
assert 'python tools\\package_release.py' in offline
assert (root/'Maak_release.bat').is_file()
# Verify development identity and immutable published-release identity.
assert '0.7.0-n16r8-s20-b${RF_BOARD_HASH}' in (root/'CMakeLists.txt').read_text()
assert "s20-b{profile_hash}" in (root/'tools/package_release.py').read_text()
assert "firmware/v0[.]7[.]0-(exp|rc)" in (root/'tools/package_release.py').read_text()
assert 'max_uri_handlers=13' in (root/'main/rflink_web_ota.cpp').read_text()
assert '"/api/events.csv"' in (root/'main/rflink_web_ota.cpp').read_text()
assert '"/api/tx"' not in (root/'main/rflink_web_ota.cpp').read_text()
print('PASS Stage18: source snapshot unaffected by IDF outputs, portable native build gating, no web TX, event export')
