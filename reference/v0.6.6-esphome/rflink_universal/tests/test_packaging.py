"""Packaging source contract for the single Windows launcher and central docs.

Actual Windows packaging is exercised by tools/test_update_release.ps1;
this static host test intentionally does not claim that PowerShell ran.
"""
from pathlib import Path
import re
ROOT = Path(__file__).resolve().parents[2]
start = ROOT / '00_RFLink_START.bat'
assert start.exists()
assert [x.name for x in ROOT.glob('*.bat')] == ['00_RFLink_START.bat']
text=start.read_text(encoding='utf8')
for marker in ('goto update_interactive','goto release','goto test_confirm','goto help',
               'update_rflink.ps1','maak_release.ps1','test_update_release.ps1'):
    assert marker in text, marker
assert 'if not "%~2"=="" goto usage' in text, 'Clean release must reject unsupported arguments'
assert 'if not defined RFLINK_TEST_NO_PAUSE pause' in text
for path in (ROOT/'docs/README.md',ROOT/'docs/START_HERE.md',ROOT/'docs/INSTALLEREN_EN_UPDATEN.md',
             ROOT/'docs/releases/V062_RELEASE_NL.md',ROOT/'rflink-universal.yaml'):
    assert path.is_file(),path
assert not (ROOT/'rflink_universal/tools/maak_upload.py').exists(), 'Obsolete Python packer must not look supported'
packer=(ROOT/'rflink_universal/tools/maak_release.ps1').read_text(encoding='utf8')
for marker in ('Assert-Directory $docsDir', 'Get-ChildItem -LiteralPath $docsDir -Recurse',
               "'00_RFLink_START.bat'", "'docs/README.md'",'Check-Identical $srcStage $zipCheck',
               "if ($file -in @('secrets.yaml'",'Move-Item -LiteralPath $zipTemp'):
    assert marker in packer,marker
assert "Copy-Stage 'rflink-universal.yaml' $bundleStage" in packer
assert 'Copy-Stage $relative $bundleStage' not in packer, 'Bundle must not recursively include docs'
print('Single launcher, central docs, SOURCE ZIP and clean ESPHome bundle guards: PASS (static)')
