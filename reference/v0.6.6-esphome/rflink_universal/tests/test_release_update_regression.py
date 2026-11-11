"""Regression checks for native-Windows release/updater contracts (does not run PowerShell).

Run 00_RFLink_START.bat and choose developer tests on Windows for actual BAT/PowerShell integration.
These host checks deliberately catch the two v2C release-script defects and the
incorrect v2B baseline / stock-decoder false conflicts.
"""
from pathlib import Path
import hashlib
import json
import re

root = Path(__file__).resolve().parents[2]
release = (root / 'rflink_universal/tools/maak_release.ps1').read_text()
update = (root / 'rflink_universal/tools/update_rflink.ps1').read_text()
win_tests = (root / 'rflink_universal/tools/test_update_release.ps1').read_text()
bat = (root / '00_RFLink_START.bat').read_text()

# Previous v2C bug: calling a function that existed only in update_rflink.ps1.
assert re.search(r'(?m)^function Read-Utf8\(', release), 'release missing its own UTF8 reader'
assert 'Read-Utf8 $configSource' in release
assert "Read-Utf8 (Join-Path $root 'rflink-universal.yaml')" not in release
assert "Read-Utf8 (Join-Path $runtimeRoot 'rflink_universal/components/rflink_universal/rflink_universal.cpp')" in release
assert "Read-Utf8 (Join-Path $runtimeRoot 'rflink_universal/components/rflink_universal/tx_policy.h')" in release
assert '-UseUpdaterOutput' not in bat, 'normal release must not silently select an update'
assert 'update_rflink.ps1' in bat and 'maak_release.ps1' in bat and 'test_update_release.ps1' in bat

# Previous updater bug: v2B was identified as TX1 due to inherited TX1 header.
expected_yaml = {
    '2B': '3F546C483866A3AC25FB290A1C72F778A8DA2BE4742A3FF5A060634C93CE64BB',
    '2C': '9EDB3EAF8F2DC3867936D8E737037E8153F53151BB388A5D6707F8E9A0D22886',
}
for version, digest in expected_yaml.items():
    name = f'v0.3.3-RC4-HF4_DECODERAPI{version}.yaml'
    path = root / 'rflink_universal/maintenance/baselines' / name
    assert hashlib.sha256(path.read_bytes()).hexdigest().upper() == digest, name
    assert name in update and digest in update, 'updater does not pin historical YAML'
assert update.index('Decoder API v2B/v2C retained') < update.index('TX1 ships under HF4 header')

# Historical stock source needs both the CPP and its matching manifest to be
# verified from the SAME version; a user's modified decoder must still stop.
registry = root / 'rflink_universal/maintenance/decoder_upgrade_baselines.json'
assert hashlib.sha256(registry.read_bytes()).hexdigest().upper() == '850CE1A24A9F577419DF53662A98FB117F03F8997185662A5BFA5032CAB9BD3A'
catalog = json.loads(registry.read_text())
assert catalog['schema'] == 1 and set(catalog['versions']) == {'2A', '2B', '2C', 'UX1', 'ITHO1'}
for version, files in catalog['versions'].items():
    assert len(files) == 16, (version, len(files))
    assert all(d == d.upper() and len(d) == 64 for d in files.values())
    assert all(name.endswith(('/decoder.cpp', '/manifest.json')) for name in files)
for filename in ['decoder.cpp', 'manifest.json']:
    fixture = root / 'rflink_universal/maintenance/baselines/decoderapi2b_esphome_builtin' / filename
    key = 'rflink_universal/decoders/esphome_builtin/' + filename
    assert hashlib.sha256(fixture.read_bytes()).hexdigest().upper() == catalog['versions']['2B'][key]
# ITHO1 is a historical baseline. Later firmware can legitimately change decoders;
# only inventory keys must still cover the official packs, not digest-equality to today.
current_pack_files={f'rflink_universal/decoders/{pack.name}/{name}' for pack in (root/'rflink_universal/decoders').iterdir() if pack.is_dir() and not pack.name.startswith('_') for name in ('decoder.cpp','manifest.json')}
assert set(catalog['versions']['ITHO1']).issubset(current_pack_files), 'Historical stock decoder registry names no longer exist'
assert 'Known-Stock-Decoder $pack.Name $oldCpp $oldManifest' in update
assert '$cppHash -ceq $files.PSObject.Properties[$pathCpp].Value' in update
assert '$manifestHash -ceq $files.PSObject.Properties[$pathManifest].Value' in update
assert 'DECODER_REVIEW' in update and 'DECODER_STOCK_UPGRADE' in update
assert 'Verified unmodified Decoder API v2B' in win_tests
assert 'Custom changes to a historical decoder must block' in win_tests

# Old Windows test had stale expected archive names and a no-op logger mutation.
assert 'RFLink_Universal_${releaseVersion}_DEFAULT.esphomebundle.tar.gz' in win_tests
assert 'RFLink_Universal_${releaseVersion}_CUSTOM.esphomebundle.tar.gz' in win_tests
assert "Replace('  log_level: INFO','  log_level: DEBUG')" in win_tests
assert "Replace('  log_level: INFO','  log_level: WARN')" in win_tests
assert 'invalid missing-radio poll' in win_tests and 'Fixture missing exact WATCHDOG1 four-argument radio health poll' in win_tests
assert 'verified historical UX2' not in win_tests
print('Release/updater static regressions: UTF8 reader, selected CUSTOM source, v2B/C baseline, verified stock upgrades, Windows test fixtures: PASS')
