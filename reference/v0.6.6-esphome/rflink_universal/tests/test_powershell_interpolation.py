"""Regression guard for an unescaped `$label:` within PowerShell double-quotes.

A real PowerShell 5.1 AST parse is also run by tools/test_update_release.ps1
on Windows. This lightweight guard is executable on any developer host.
"""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
ps_files = list((ROOT/'tools').glob('*.ps1'))
assert ps_files
invalid = []
for path in ps_files:
    for line_no, line in enumerate(path.read_text(encoding='utf-8-sig').splitlines(), 1):
        # This deliberately catches the ambiguous sequence in interpolated text.
        # Valid ${label}: and $env:NAME are not flagged.
        for m in re.finditer(r'\$([A-Za-z_][A-Za-z0-9_]*):(?=[^\w{])',line):
            invalid.append(f'{path.name}:{line_no}: {m.group()}')
assert not invalid, 'Ambiguous PowerShell variable-colon expressions: '+', '.join(invalid)
s=(ROOT/'tools/update_rflink.ps1').read_text(encoding='utf-8')
for name in ('EXPLICIT_FIRMWARE_OVERRIDE','EXPLICIT_RELEASE','CONFLICT'):
    assert name+' ${label}:' in s
assert 'PowerShell syntaxfout' in (ROOT/'tools/test_update_release.ps1').read_text(encoding='utf-8')
print('PowerShell interpolation regression PASS:', len(ps_files), 'scripts')
