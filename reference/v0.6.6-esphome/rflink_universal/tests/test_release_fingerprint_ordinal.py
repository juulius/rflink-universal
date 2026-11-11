"""v0.5.3: check packer's culture-independent source-hash contract.

This runs as a developer test without PowerShell; Windows packer still needs no Python.
"""
from pathlib import Path
import hashlib
import re

ROOT=Path(__file__).resolve().parents[1]
ps=(ROOT/'tools/maak_release.ps1').read_text(encoding='utf-8')
identity=(ROOT/'components/rflink_universal/firmware_identity.h').read_text(encoding='utf-8')
assert '[Array]::Sort($sortedRecords, [StringComparer]::Ordinal)' in ps
assert 'Sort-Object -CaseSensitive)' not in ps
assert '$computed -cne $hashMatch.Groups[1].Value' in ps, 'must remain fail-closed'
assert '$version' in ps, 'release number must be tied to identity'
files=[p for name in ('components','decoders','profiles','ui','options')
       for p in (ROOT/name).rglob('*') if p.is_file()
       and p.name!='firmware_identity.h'
       and p.suffix.lower() in ('.cpp','.h','.json','.html','.yaml','.py','.svg','.ico')]

def hash_records(mapping):
    records=[f'{name} {hashlib.sha256(blob).hexdigest()}' for name,blob in mapping.items()]
    return hashlib.sha256(('\n'.join(sorted(records))+'\n').encode('utf-8')).hexdigest()[:12]

source={p.relative_to(ROOT).as_posix():p.read_bytes() for p in files}
actual=hash_records(source)
match=re.search(r'RFLINK_FIRMWARE_BUILD_HASH\s*=\s*"([a-f0-9]{12})"',identity)
assert match and match.group(1)==actual,(match.group(1) if match else None,actual)
# Intentionally mutate one runtime file in memory to prove the algorithm catches content edits.
changed=dict(source)
key='ui/index.html'
changed[key]=changed[key]+b'\n<!-- deliberate mutation -->\n'
assert hash_records(changed)!=actual,'Hash check must reject any content change'
# Input order cannot change digest, regardless of enumeration order.
assert hash_records(dict(reversed(list(source.items()))))==actual
print('RELEASE v0.5.3: ordinal hash, order invariance, modified-source rejection PASS:',actual,len(files))
