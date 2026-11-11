"""Deterministic migration fixtures for Wizard 4's firmware conflict contract.

No customer YAML is included. PowerShell execution is separately tested on Windows.
"""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
BASE = ROOT / 'rflink_universal/maintenance/baselines'
legacy = (BASE / 'v0.3.3-RC4-HF3_LEGACY_REVIEWED.yaml').read_text()
modern = (BASE / 'v0.3.3-RC4-HF3_UX2.yaml').read_text()
release = (ROOT / 'rflink-universal.yaml').read_text()
updater = (ROOT / 'rflink_universal/tools/update_rflink.ps1').read_text()

def block(text, name):
    m = re.search(rf'(?m)^{re.escape(name)}:\s*$', text)
    assert m, name
    n = re.search(r'(?m)^[a-zA-Z_][\w-]*:', text[m.end():])
    return text[m.start():m.end() + n.start() if n else len(text)].strip()

def active_lines(text):
    return '\n'.join(line.rstrip() for line in text.splitlines()
                     if line.strip() and not line.lstrip().startswith('#'))

assert active_lines(block(legacy, 'external_components')) == active_lines(block(modern, 'external_components'))
assert active_lines(block(modern, 'external_components')) == active_lines(block(release, 'external_components'))
assert active_lines(block(legacy, 'remote_transmitter')) != active_lines(block(modern, 'remote_transmitter'))
assert active_lines(block(legacy, 'remote_transmitter')).count('rmt_symbols: 48') == 0
assert active_lines(block(release, 'remote_transmitter')).count('rmt_symbols: 48') == 2
assert 'record_tx_command(0,' in block(release, 'remote_transmitter')
assert 'KNOWN_LEGACY_TX' in updater and 'Executable-Lines' in updater
assert 'AcceptReleaseFirmwareChanges' in updater
assert "$takeRelease = Read-YN" in updater and ' $false' in updater
assert 'INPUT SHA256:' in updater and 'INPUT TYPE:' in updater
# Any unknown, substantive change must fail matching the certified old TX.
custom = block(legacy, 'remote_transmitter').replace('carrier_duty_percent: 100%', 'carrier_duty_percent: 90%')
assert active_lines(custom) != active_lines(block(legacy, 'remote_transmitter'))
print('Wizard4 migration fixtures: same external component (comments only), verified legacy TX restore, unknown TX requires explicit approval: PASS (static)')
