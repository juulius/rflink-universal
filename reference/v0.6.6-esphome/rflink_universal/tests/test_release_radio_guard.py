"""Regression: WATCHDOG1 four-argument missing-radio health polls pass release validation.

Runs the same three .NET-compatible regular expressions extracted from the
PowerShell release script against the shipped DEFAULT YAML, then corrupts
several health signals to confirm the check fails closed. Native Windows
PowerShell execution is independently covered by test_update_release.ps1.
"""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[2]
release = (root / 'rflink_universal/tools/maak_release.ps1').read_text(encoding='utf-8-sig')
yaml = (root / 'rflink-universal.yaml').read_text(encoding='utf-8')

assert "Selected YAML lacks missing-radio detection; re-run YAML updater" not in release, 'obsolete three-argument check is still present'
assert "(?ms)^interval:[ \\t]*\\r?\\n" in release, 'release does not scope health checks to interval'
assert "[regex]::Matches($intervalMatch.Groups[1].Value" in release
assert 'if ($count -ne 1)' in release
pattern_block = re.search(r'\$radioHealthPatterns = @\(\s*(.*?)\s*\n    \)', release, re.S)
assert pattern_block, 'missing radio health validation list'
patterns = re.findall(r"'([^']+)'", pattern_block.group(1))
assert len(patterns) == 3, len(patterns)
assert all(f'set_radio_available\\(\\s*{i}' in pat for i, pat in enumerate(patterns))
assert 'rx_433_42' in patterns[0] and 'tx_433_42' in patterns[0] and 'radio_watchdog_healthy\\(\\s*0' in patterns[0]
assert 'rx_433_92' in patterns[1] and 'tx_433_92' in patterns[1] and 'radio_watchdog_healthy\\(\\s*1' in patterns[1]
assert 'cc1101_868' in patterns[2] and patterns[2].count('true') == 2 and 'radio_watchdog_healthy\\(\\s*2' in patterns[2]
assert '868MHz Itho SPI packet profile missing' not in release, 'release packer is still coupled to old Itho YAML profile'
assert 'Radio 3 is SPI/packet-capable' in release

def validates(text: str) -> bool:
    # .NET's regex \z anchor is Python's \Z. Both recognize EOF here.
    match = re.search(r'(?ms)^interval:[ \t]*\r?\n(.*?)(?=^[A-Za-z_][A-Za-z_0-9]*:|\Z)', text)
    if not match:
        return False
    section = match.group(1)
    return all(len(re.findall(r'(?m)^[ \t]+' + pat + r'[ \t]*$', section)) == 1 for pat in patterns)

assert validates(yaml), 'shipped DEFAULT YAML is rejected by corrected radio guard'
lines = [line for line in yaml.splitlines() if 'id(rflink_core).set_radio_available(' in line]
assert len(lines) == 3
# Missing/modified health signal on every radio, duplicates and commented calls.
checks = [
    yaml.replace(lines[0], ''),
    yaml.replace('!id(cc1101_433_42).is_failed()', 'true'),
    yaml.replace('!id(rx_433_42).is_failed(), !id(tx_433_42).is_failed()', 'true, true'),
    yaml.replace('!id(cc1101_433_92).is_failed() && id(rflink_core).radio_watchdog_healthy(1), !id(rx_433_92).is_failed()', 'true, true'),
    yaml.replace('!id(cc1101_868).is_failed() && id(rflink_core).radio_watchdog_healthy(2), true, true', '!id(cc1101_868).is_failed() && id(rflink_core).radio_watchdog_healthy(2), false, true'),
    yaml.replace('radio_watchdog_healthy(0)', 'radio_watchdog_healthy(1)', 1),
    yaml.replace(lines[0], lines[0] + '\n' + lines[0]),
    yaml.replace(lines[0], '          # ' + lines[0].strip()),
    yaml.replace('interval:\n', '# interval:\n'),
]
for case_no, altered in enumerate(checks, 1):
    assert altered != yaml, ('invalid test setup', case_no)
    assert not validates(altered), ('unsafe release accepted mutated YAML', case_no)

print('DEFAULT release radio guard: 3 WATCHDOG1 four-argument polls accepted; 9 invalid/missing/duplicate/wrong-watchdog mutations rejected: PASS')
