#!/usr/bin/env python3
"""Developer convenience; actual ESPHome generation runs automatically in __init__.py."""
import importlib.util
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
C=ROOT/'components/rflink_universal'
spec=importlib.util.spec_from_file_location('manifest_schema', C/'manifest_schema.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
packs=[]
for folder in sorted((ROOT/'decoders').iterdir()):
    if folder.is_dir() and not folder.name.startswith('_'):
        manifest=m.load_manifest(folder/'manifest.json')
        if manifest.get('enabled',True): packs.append((folder,manifest))
header=m.generate_header(packs)
(C/'capability_catalog_generated.h').write_text(header,encoding='utf-8')
print('Generated',len(packs),'decoder packs,',sum(len(v['capabilities']) for _,v in packs),'protocols')

import importlib.util
spec2=importlib.util.spec_from_file_location("radio_contract_v2", C/"radio_contract_v2.py")
rm=importlib.util.module_from_spec(spec2);spec2.loader.exec_module(rm)
(C/"radio_v2_catalog_generated.h").write_text(rm.generate_contract_header(packs),encoding="utf-8")
