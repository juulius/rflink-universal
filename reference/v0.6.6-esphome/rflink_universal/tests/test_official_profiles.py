"""Built-in device-profile catalog must stay source-backed and decoder-safe."""
import importlib.util, json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('generate_profiles',ROOT/'tools/generate_profiles.py')
mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod)
profiles=mod.load_profiles()
assert len(profiles)==26, len(profiles)
assert len([p for p in profiles if p['manufacturer']=='Orcon'])==16
ids={p['id'] for p in profiles}
assert {'acurite_609txc','alecto_v1_weather','fineoffset_wh2','itho_rft_5360124','byron_sx21','nexa_selflearn','dooya_ac12316','rubicson_tfa303197','somfy_telis1_rts','somfy_telis4_rts','orcon_mvs15r_15rf'}.issubset(ids)
assert all(p['hardware_validation']=='pending_on_rflink_universal' for p in profiles)
assert not any(p['protocol'] in {'KeeLoq','RCSwitch'} for p in profiles), 'generic/rolling protocols must not get speculative product profiles'
by_id={p['id']:p for p in profiles}
assert by_id['fineoffset_wh2']['models']==['Fine Offset WH2','ClimeMET CM9088']
assert 'InFactory' not in ' '.join(by_id['rubicson_tfa303197']['models'])
assert by_id['somfy_telis1_rts']['expected_actions']==['up','down','my','prog']
assert by_id['orcon_mvs15r_15rf']['protocol']=='RamsesII'
assert by_id['orcon_mvs15r_15rf']['compatibility']=='manual_only'
assert 'std::strcmp(official[i].compatibility, "manual_only") == 0' in (ROOT/'components/rflink_universal/rflink_universal.cpp').read_text()
assert by_id['itho_rft_5360124']['expected_actions']==['low','medium','high','timer_10','timer_20','timer_30']
for p in profiles:
    assert p['sources'] and all(s['url'].startswith('https://') for s in p['sources'])
    assert set(p['learn_help'])=={'en','nl','fr','de'} and set(p['notes'])=={'en','nl','fr','de'}
expected=mod.generate(profiles)
actual=(ROOT/'components/rflink_universal/profile_catalog_generated.h').read_text(encoding='utf-8')
assert actual==expected, 'generated profile catalog is stale; run tools/generate_profiles.py'
print('Official device profiles: 26 profiles (16 Orcon), decoder/action validation, locale completeness and generated header sync PASS')
