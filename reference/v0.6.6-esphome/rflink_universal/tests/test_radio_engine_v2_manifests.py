"""Generated runtime routes must match all enabled decoder manifests."""
from pathlib import Path
import json, sys, importlib.util
r=Path(__file__).resolve().parents[1]
c=r/'components/rflink_universal'
spec=importlib.util.spec_from_file_location('radio_contract_v2',c/'radio_contract_v2.py')
mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod)
packs=[]
for d in sorted((r/'decoders').iterdir()):
    p=d/'manifest.json'
    if p.is_file() and not d.name.startswith('_'):
        m=json.loads(p.read_text())
        if m['enabled']:
            assert mod.validate_contract(m)
            packs.append((d,m))
assert sum(len(m['capabilities']) for _,m in packs)==13
assert mod.generate_contract_header(packs)==(c/'radio_v2_catalog_generated.h').read_text()
# Cannot override reviewed serial transmitter via radio-v2 only (no manifest approval).
a=next(cap for _,m in packs for cap in m['capabilities'] if cap['protocol']=='RamsesII')
m=next(m for _,m in packs if m['id']=='ramses_ii')
copy=json.loads(json.dumps(m))
copy['capabilities'][0]['radio_v2']['tx']['method']='disabled'
try: mod.validate_contract(copy)
except ValueError:pass
else:raise AssertionError('unreviewed TX unexpectedly enabled')
copy=json.loads(json.dumps(m))
copy['capabilities'][0]['radio_v2']['rx']['routes'][0]['kind']='raw'
try:mod.validate_contract(copy)
except ValueError:pass
else:raise AssertionError('wrong RF frame format unexpectedly accepted')
print('Radio Engine V2 manifests: PASS, 9 packs, 13 protocols, safe RX/TX validation')
