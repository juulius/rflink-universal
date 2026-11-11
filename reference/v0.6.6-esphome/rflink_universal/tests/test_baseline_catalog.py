"""Verify embedded original UX2 baselines against known source-archive checksums."""
from pathlib import Path
import hashlib
import re

root = Path(__file__).resolve().parents[2]
folder = root / 'rflink_universal/maintenance/baselines'
expected = {
    'v0.3.3-RC4-HF3_UX2.yaml': 'f1f9174559cfb8863989f0f24923384c55a21da4562b03e941392b50d0d3df4e',
    'v0.3.3-RC4-HF4_UX2.yaml': '22a4bf525499043a53af943956bd734277b5a951a650dbe7f60227d481681879',
}
for name, digest in expected.items():
    p = folder / name
    data = p.read_bytes()
    assert hashlib.sha256(data).hexdigest() == digest, name
    text = data.decode('utf-8')
    assert re.search(r'^# RFLink Universal v0\.3\.3 RC4-HF[34]', text), name
    assert '# RADIO-OPTIONAL UPDATE:' in text and 'rmt_symbols: 96' in text
    assert 'rmt_symbols: 48' in text and 'led_ota(true)' in text

fixed = folder / 'v0.3.3-RC4-HF4_COMPILEFIX1.yaml'
assert hashlib.sha256(fixed.read_bytes()).hexdigest().upper() == 'D4BB34C1E23A3E7EBACF31D3F6C26B32DCAAB86426E539DCD575BC01A656B762'
ha = folder / 'v0.3.3-RC4-HF4_HA1.yaml'
assert ha.read_bytes() != (root / 'rflink-universal.yaml').read_bytes(), 'historical HA1 must remain immutable'
assert hashlib.sha256(ha.read_bytes()).hexdigest().upper() == '7A9D8688292D763FEF6C92D95F214584886966335D8C199CAF2503C57F2E9A9F'
assert ha.read_text().count('# HOME_ASSISTANT_BRIDGE1:') == 1
config1 = folder / 'v0.3.3-RC4-HF4_CONFIG1.yaml'
assert config1.read_bytes() != (root / 'rflink-universal.yaml').read_bytes(), 'historical CONFIG1 must remain immutable'
assert hashlib.sha256(config1.read_bytes()).hexdigest().upper() == '5039F2558DB37C6258906461B81E523334681566EB5AB49C5D3B1159EA950DEE'

tx1 = folder / 'v0.3.3-RC4-HF4_TX1.yaml'
assert hashlib.sha256(tx1.read_bytes()).hexdigest().upper() == 'A012633BD5E3FDB3AB8424B218958340C9A2A9838D57B5C7BF741702DDE6AEBA'
assert tx1.read_bytes() != (root / 'rflink-universal.yaml').read_bytes(), 'historical TX1 must remain immutable' 

assert fixed.read_text().count('id(rflink_status_rgb).loop();') == 4

new = (root / 'rflink-universal.yaml').read_text()
assert 'board_revision: eigen_rfid_board_rev2_gpio14' in new
assert 'radio3_csn: GPIO15' in new
assert 'rmt_symbols: 96' in new and 'rmt_symbols: 48' in new
updater = (root / 'rflink_universal/tools/update_rflink.ps1').read_text()
for name in expected:
    assert name in updater
assert 'Resolve-KnownBaseline $oldRelease $oldText' in updater
assert 'v0.3.3-RC4-HF3_LEGACY_REVIEWED.yaml' in updater
assert 'v0.3.3-RC4-HF4_COMPILEFIX1.yaml' in updater and 'isCompilefix1' in updater
assert "if ($baseRelease -cne $oldRelease)" in updater
assert "Baseline selection: " in updater
assert "if ($oldRelease -ceq $newReleaseVersion -and $oldRev)" not in updater
assert 'v0.3.3-RC4-HF4_TX1.yaml' in updater and 'A012633BD5E3FDB3AB8424B218958340C9A2A9838D57B5C7BF741702DDE6AEBA' in updater
assert 'v0.3.3-RC4-HF4_CONFIG1.yaml' in updater and '5039F2558DB37C6258906461B81E523334681566EB5AB49C5D3B1159EA950DEE' in updater
assert "v0.3.3-RC4-HF4_HA1.yaml" in updater and '7A9D8688292D763FEF6C92D95F214584886966335D8C199CAF2503C57F2E9A9F' in updater
print('Historical baselines: HF3/HF4 source identity, checksums, safe fallback and release pinout: PASS (static)')
