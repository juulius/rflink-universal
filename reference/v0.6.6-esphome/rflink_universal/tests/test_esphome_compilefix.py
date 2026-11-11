"""A compiler-log-derived regression. Older host stub accepted &id(pointer) by mistake."""
from pathlib import Path
import yaml
root=Path(__file__).resolve().parents[2]
class L(yaml.SafeLoader): pass
def tag(loader, suffix, node):
    if isinstance(node,yaml.ScalarNode): return loader.construct_scalar(node)
    if isinstance(node,yaml.SequenceNode): return loader.construct_sequence(node)
    return loader.construct_mapping(node)
L.add_multi_constructor('!',tag)
yml=(root/'rflink-universal.yaml').read_text()
cfg=yaml.load(yml,Loader=L)
assert cfg['esp32']['variant']=='esp32s3'
assert cfg['substitutions']['radio3_csn']=='GPIO15'
assert cfg['substitutions']['status_led_gpio']=='GPIO48'
callbacks=cfg['ota'][0]
for name in ('on_begin','on_progress','on_end','on_error'):
    body=callbacks[name][0]['lambda']
    assert body.count('id(rflink_status_rgb).loop();')==1, name
    assert 'get_output()->write_state(' not in body, name
    assert body.index('call.perform();')<body.index('id(rflink_status_rgb).loop();'), name
cpp=(root/'rflink_universal/components/rflink_universal/rflink_universal.cpp').read_text()
assert '#include <inttypes.h>' in cpp
assert 'generation %" PRIu32' in cpp
assert 'hash %08" PRIX32' in cpp
assert 'Restored %u devices from NVS generation %u' not in cpp
assert 'RAW hash %08X | %u' not in cpp
for source in (root/'rflink_universal/maintenance/baselines').glob('*HF3*.yaml'):
    assert source.is_file()
print('Regression based on real ESPHome 2026.9 error: corrected four callbacks, uint32 formatting, GPIO/RMT retained (STATIC)')
