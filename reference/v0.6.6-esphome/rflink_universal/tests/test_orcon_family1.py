"""Orcon family safety and UI source contract tests."""
import json, subprocess, re
from pathlib import Path
R=Path(__file__).resolve().parents[1]
profiles={j['id']:j for path in (R/'profiles').glob('orcon*.json') for j in [json.loads(path.read_text())]}
assert len(profiles)==16
assert 'orcon_mvs15r_15rf' in profiles
assert profiles['orcon_mvs15r_15rf']['controls']==['away','auto','low','medium','high','timer_15','timer_30','timer_60']
for p in profiles.values():
 assert p['protocol']=='RamsesII'
 assert p['hardware_validation']=='pending_on_rflink_universal'
 assert p['suggestion_policy'] in ('manual_select','protocol_candidate')
 assert all(len(p['setup_steps'][lang])>=2 for lang in ('en','nl','fr','de'))
 assert all(isinstance(x,str) and x for x in p['controls'])
 if 'sensor' in p['device_type']:assert not p['controls']
assert [p['id'] for p in profiles.values() if p['suggestion_policy']=='protocol_candidate']==['orcon_15rf_general']
assert not any('bypass_' in c for p in profiles.values() if not p['id'].startswith('orcon_hrc') for c in p['controls'])
ui=(R/'ui/index.html').read_text()
assert "$$('[data-device-profile]').forEach" in ui
assert "$$('[data-profile-command]').forEach" in ui
assert 'profileSetupHtml(x)' in ui and 'manufacturers.map(brand=>' in ui and 'data-pair-open' in ui
assert all('profileflow.command.timer_'+str(n) in json.loads((R/'ui'/'locales'/(lang+'.json')).read_text()) for lang in ('nl','en','fr','de') for n in (15,30,60))
manifest=json.loads((R/'decoders/ramses_ii/manifest.json').read_text())
# Strictly opt-in experimental control: an encoder is available to the engine,
# but hardware, pairing state and explicit owner permission remain mandatory.
tx=manifest['capabilities'][0]['transmit']
assert tx['encoder']=='ramses_ii' and tx['status']=='host_tested'
assert tx['default_enabled'] is False
assert tx['radio']=='868.30'
assert [a['id'] for a in tx['actions']]==profiles['orcon_mvs15r_15rf']['controls']
assert set(tx['required_fields'])=={'ramses_controller','ramses_fan','ramses_paired'}
assert manifest['capabilities'][0]['radio_v2']['tx']['transport']['preferred']=='gdo_serial'
print('ORCONFAMILY1 profile/controls/unknown-id/manual-select/experimental-TX-gates/UI-hooks validation PASS')
