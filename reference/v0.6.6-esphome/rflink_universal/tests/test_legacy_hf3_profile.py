"""Exact-profile regression for an older HF3 that Wizard 2 rejected.

Only the redacted reference is distributed; a private user YAML is never packaged.
PowerShell execution must additionally be tested on Windows.
"""
from hashlib import sha256
from pathlib import Path
import re
ROOT = Path(__file__).resolve().parents[2]
legacy = ROOT/'rflink_universal/maintenance/baselines/v0.3.3-RC4-HF3_LEGACY_REVIEWED.yaml'
original = legacy.read_text()
expected = '5EEC740606CC9D85D0547F527B16A7B18219BF73AF2E3B8188F8209F36F974FC'
assert sha256(legacy.read_bytes()).hexdigest().upper() == expected
keys = ('name','friendly_name','itho_id1','itho_id2','itho_id3','itho_device_type')
pattern = r'(?m)^(  (?:' + '|'.join(keys) + r'):)[^\n]*$'
assert original.count('__PERSONAL_SETTING__') == 6
assert not any(s in original for s in ("itho_id1: '0x06'", "itho_id2: '0xCF'", "itho_id3: '0x4A'"))

def canonicalize(text):
    text = text.replace('\r\n','\n').replace('\r','\n')
    cut = text.find('\npackages:')
    assert cut >= 0
    a,b = text[:cut],text[cut:]
    assert len(re.findall(pattern,a)) == 6
    return re.sub(pattern,lambda m: m.group(1)+' __PERSONAL_SETTING__',a)+b

assert canonicalize(original) == original
sample = original
for key,value in [('name','synthetic-device'),('friendly_name','Synthetic Device'),
                  ('itho_id1',"'0xAA'"),('itho_id2',"'0xBB'"),
                  ('itho_id3',"'0xCC'"),('itho_device_type',"'0x16'")]:
    sample = sample.replace(f'  {key}: __PERSONAL_SETTING__', f'  {key}: {value}',1)
assert sample.count('__PERSONAL_SETTING__') == 0
assert sha256(canonicalize(sample).encode()).hexdigest().upper() == expected
assert sha256(canonicalize(sample.replace('  dedupe_window: 80ms','  dedupe_window: 90ms')).encode()).hexdigest().upper() != expected
assert sha256(canonicalize(sample.replace('    cs_pin: GPIO15','    cs_pin: GPIO9')).encode()).hexdigest().upper() != expected
assert 'rmt_symbols: 48' not in sample and sample.count('rmt_symbols: 96') == 2
release = (ROOT/'rflink-universal.yaml').read_text()
assert release.count('rmt_symbols: 48') == 2
assert release.count('rmt_symbols: 96') == 2
assert release.count('id(rflink_status_rgb).loop();') == 4
assert 'write_state(&id(' not in release
assert 'radio3_csn: GPIO15' in release
updater = (ROOT/'rflink_universal/tools/update_rflink.ps1').read_text()
assert expected in updater and 'Legacy-HF3-Canonical $oldText' in updater
assert 'reviewed legacy HF3 variant' in updater and "return $legacyFile" in updater
assert "if ($version -ceq 'v0.3.3-RC4-HF3'" in updater and 'Text-Sha256 $canonical' in updater
assert 'if ($version -ceq' in updater and 'Full legacy HF3 profile verified.' in updater
print('Legacy HF3: redacted reference, identity-tolerant exact signature, altered firmware rejection, HF4 RMT/OTA/pinout checks PASS (static)')
