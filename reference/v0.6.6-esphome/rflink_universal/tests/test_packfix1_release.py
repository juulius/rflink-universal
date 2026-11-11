"""PACKFIX1 contract: Windows release guard matches current LOGBOOK1 UI/API.

Runs on host without PowerShell; native Windows packaging remains a separate test.
"""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[2]
ps = (root / 'rflink_universal/tools/maak_release.ps1').read_text(encoding='utf-8-sig')
html = (root / 'rflink_universal/ui/index.html').read_text(encoding='utf-8')
cpp = (root / 'rflink_universal/components/rflink_universal/rflink_universal.cpp').read_text(encoding='utf-8')

assert "data-i18n=\"static.038\"" not in ps, 'obsolete UI marker still blocks packing'
match = re.search(r"# PACKFIX1 / LOGBOOK1:.*?Fail 'LOGBOOK1 radio/system/web runtime missing.*?\n    }", ps, re.S)
assert match, 'new fail-closed check not found'
guard = match.group(0)
ui = re.findall(r"\$html\.Contains\('([^']+)'\)", guard)
core = re.findall(r"\$cpp\.Contains\('([^']+)'\)", guard)
assert len(ui) >= 6 and len(core) >= 4
assert all(token in html for token in ui), [x for x in ui if x not in html]
assert all(token in cpp for token in core), [x for x in core if x not in cpp]
assert 'id="monitorLogType"' in html and 'id="monitorExport"' in html, 'radio log controls missing'
assert 'id="systemLogLimit"' in html and 'id="webLogLimit"' in html, 'log settings missing'

# Negative cases must fail the same marker check, not pass on legacy strings.
def passes_ui(t): return all(marker in t for marker in ui)
def passes_core(t): return all(marker in t for marker in core)
assert passes_ui(html) and passes_core(cpp)
for marker in ui:
    assert not passes_ui(html.replace(marker, 'BROKEN_MARKER')), marker
for marker in core:
    assert not passes_core(cpp.replace(marker, 'BROKEN_MARKER')), marker
print(f'PACKFIX1: {len(ui)} current UI + {len(core)} firmware markers verified; negative tests PASS')
