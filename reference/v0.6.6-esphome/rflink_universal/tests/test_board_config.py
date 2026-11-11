"""Validate the actual delivered YAML against the confirmed new PCB map."""
from pathlib import Path
import yaml

ROOT = Path(__file__).resolve().parents[2]
class Loader(yaml.SafeLoader):
    pass

def unknown_tag(loader, _suffix, node):
    if isinstance(node, yaml.ScalarNode):
        return loader.construct_scalar(node)
    if isinstance(node, yaml.SequenceNode):
        return loader.construct_sequence(node)
    return loader.construct_mapping(node)

Loader.add_multi_constructor('!', unknown_tag)
cfg = yaml.load((ROOT / 'rflink-universal.yaml').read_text(), Loader=Loader)
# Resolve the substitutions used by ESPHome, not a second fabricated reference map.
import re
sub = cfg['substitutions']
def pin(value):
    match = re.fullmatch(r'\$\{([a-zA-Z0-9_]+)\}', value)
    assert match, f'Hardcoded or invalid pin reference: {value!r}'
    key = match.group(1)
    assert key in sub, f'Undefined pin label: {key}'
    return sub[key].split(' #', 1)[0]

expected = {
    'spi_sck': 'GPIO12', 'spi_mosi': 'GPIO11', 'spi_miso': 'GPIO13',
    'radio1_csn': 'GPIO4', 'radio1_gdo0': 'GPIO5', 'radio1_gdo2': 'GPIO6',
    'radio2_csn': 'GPIO9', 'radio2_gdo0': 'GPIO8', 'radio2_gdo2': 'GPIO14',
    'radio3_csn': 'GPIO15', 'radio3_gdo0': 'GPIO16', 'radio3_gdo2': 'GPIO17',
    'status_led_gpio': 'GPIO48',
}
assert {key: sub[key].split(' #', 1)[0] for key in expected} == expected, 'HF4 default PCB GPIO map differs'
assert [pin(cfg['spi'][field]) for field in ('clk_pin','mosi_pin','miso_pin')] == ['GPIO12','GPIO11','GPIO13']
assert [pin(r['cs_pin']) for r in cfg['cc1101']] == ['GPIO4','GPIO9','GPIO15']
assert pin(cfg['cc1101'][2]['gdo0_pin']) == 'GPIO16'
assert [pin(r['pin']['number'] if isinstance(r['pin'], dict) else r['pin']) for r in cfg['remote_receiver']] == ['GPIO6','GPIO14']
assert all(r['pin']['mode']['pulldown'] for r in cfg['remote_receiver']), 'Unconnected receiver input floats'
assert [pin(r['pin']) for r in cfg['remote_transmitter']] == ['GPIO5','GPIO8']
used = [pin(cfg['spi'][s]) for s in ('mosi_pin','clk_pin','miso_pin')]
used += [pin(r['cs_pin']) for r in cfg['cc1101']]
used += [pin(cfg['cc1101'][2]['gdo0_pin'])]
used += [pin(r['pin']['number'] if isinstance(r['pin'], dict) else r['pin']) for r in cfg['remote_receiver']]
used += [pin(r['pin']) for r in cfg['remote_transmitter']]
used += [sub['radio3_gdo2'].split(' #', 1)[0]]  # reserved 868 connector; collision still forbidden
assert len(used) == len(set(used)), 'GPIO-conflict between configured or reserved roles'
assert not set(used).intersection(('GPIO35', 'GPIO36')), 'N16R8 reserved PSRAM GPIO in use'
assert sub['status_led_gpio'] not in used, 'WS2812 GPIO conflicts with a CC1101 pin'
assert cfg['cc1101'][2]['packet_mode'] is True
assert cfg['cc1101'][2]['modulation_type'] == '2-FSK'
# Real YAML only: all active direct pin settings must reference the central table.
for line in (ROOT / 'rflink-universal.yaml').read_text().splitlines():
    if line.lstrip().startswith('#') or line.startswith('  ') and re.match(r'^  [A-Za-z0-9_]+: GPIO\d+', line):
        continue
    assert not re.search(r'(?<![A-Za-z0-9_])GPIO\d+', line), f'GPIO literal outside central substitutions: {line}'
assert cfg['packages']['rgb_status'] == 'rflink_universal/options/rgb_status_ws2812.yaml', 'RGB package is not enabled'
rgb = yaml.load((ROOT / 'rflink_universal/options/rgb_status_ws2812.yaml').read_text(), Loader=Loader)
assert rgb['light'][0]['pin'] == '${status_led_gpio}'
assert rgb['light'][0]['internal'] is True
assert rgb['light'][0]['num_leds'] == 1
assert rgb['light'][0]['platform'] == 'esp32_rmt_led_strip'
assert rgb['light'][0]['chipset'].upper() == 'WS2812'
assert rgb['light'][0]['channel_colors'] == 'GRB'
assert cfg['substitutions']['status_led_gpio'] not in used, 'WS2812 GPIO conflicts with a CC1101 pin'
assert rgb['light'][0]['restore_mode'] == 'ALWAYS_OFF'
assert 'on_connect' in rgb['wifi'] and 'on_disconnect' in rgb['wifi']
assert 'led_wifi(true)' in rgb['wifi']['on_connect'][0]['lambda']
assert 'led_wifi(false)' in rgb['wifi']['on_disconnect'][0]['lambda']
assert rgb['interval'][0]['interval'] == '100ms'
animation = rgb['interval'][0]['then'][0]['lambda']
assert 'status_led().sample(millis())' in animation
assert 'rflink_status_rgb' in animation
assert all(k in cfg['ota'][0] for k in ('on_begin', 'on_progress', 'on_end', 'on_error'))
assert 'led_ota(true)' in cfg['ota'][0]['on_begin'][0]['lambda']
assert 'led_ota(false)' in cfg['ota'][0]['on_error'][0]['lambda']
assert all(any('led_tx()' in action.get('lambda', '') for action in tx['on_transmit']['then'][0]['if']['then']) for tx in cfg['remote_transmitter']), 'TX indicator and missing-radio guards must be nested together'
core=(ROOT/'rflink_universal/components/rflink_universal/rflink_universal.cpp').read_text()
assert 'led_.pair_start(millis())' in core and 'led_.pair_end(millis())' in core
assert 'script' not in cfg, 'legacy YAML Itho scripts must not return'
print('HF4 actual YAML: central substitutions, radio pinout/no-overlap, reserved pins, WS2812 GPIO48 and LED wiring: PASS')
