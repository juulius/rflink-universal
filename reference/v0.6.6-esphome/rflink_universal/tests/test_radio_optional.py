"""Regression: optional CC1101s must never masquerade as online or be used when failed.

Source-structure and config checks only; not a hardware boot or ESPHome firmware build.
"""
from pathlib import Path
import json
import yaml

root = Path(__file__).resolve().parents[2]
class Loader(yaml.SafeLoader):
    pass

def tagged(loader, suffix, node):
    if isinstance(node, yaml.ScalarNode):
        return loader.construct_scalar(node)
    if isinstance(node, yaml.SequenceNode):
        return loader.construct_sequence(node)
    return loader.construct_mapping(node)

Loader.add_multi_constructor('!', tagged)
cfg = yaml.load((root / 'rflink-universal.yaml').read_text(), Loader=Loader)
core = (root / 'rflink_universal/components/rflink_universal/rflink_universal.cpp').read_text()
header = (root / 'rflink_universal/components/rflink_universal/rflink_universal.h').read_text()
html = (root / 'rflink_universal/ui/index.html').read_text()
assert len(cfg['cc1101']) == 3
poll = cfg['interval'][0]['then'][0]['lambda']
for i, radio in enumerate(cfg['cc1101']):
    if i < 2:
        suffix = '42' if i == 0 else '92'
        assert f'set_radio_available({i}, !id({radio["id"]}).is_failed() && id(rflink_core).radio_watchdog_healthy({i}), !id(rx_433_{suffix}).is_failed(), !id(tx_433_{suffix}).is_failed())' in poll
    else:
        assert f'set_radio_available(2, !id({radio["id"]}).is_failed() && id(rflink_core).radio_watchdog_healthy(2), true, true)' in poll
assert 'bool checked{false};' in header and 'bool available{false};' in header
assert 'void RFLinkUniversal::set_radio_available(' in core
assert '"not detected (RF disabled; network services remain active)"' in core
assert '"available\\\":" << bool_json(ch.available)' in core
assert '!channels_[channel].rx_ready' in core
assert '"radiomodule niet aangesloten"' in core
nl = json.loads((root / 'rflink_universal/ui/locales/nl.json').read_text())
assert 'r.available?' in html and "rfTranslate('dyn.not_connected')" in html
assert "rfTranslate('dyn.checking_short')" in html
assert nl['dyn.not_connected'] == 'Niet aangesloten' and nl['dyn.checking_short'] == 'Controleren'
assert all(isinstance(r['pin'], dict) and r['pin']['mode'].get('pulldown') for r in cfg['remote_receiver'])
for tx in cfg['remote_transmitter']:
    first = tx['on_transmit']['then'][0]['if']
    assert tx['id'].replace('tx_', 'cc1101_') in first['condition']['lambda']
    assert any('cc1101.begin_tx' in step for step in first['then'])
    for action in tx['on_complete']['then']:
        if 'if' in action:
            assert 'is_failed()' in action['if']['condition']['lambda']
assert 'script' not in cfg, 'legacy direct Itho scripts must not bypass the native device TX guard'
assert '868.30 MHz packet radio is unavailable' in core
assert 'native_plan_available_(availability)' in core
assert 'packet_transmitter_->transmit_packet(packet)' in core
print('Optional radios: 3 probes, input and transmit guards, status/UI, pin pulldowns: PASS')
