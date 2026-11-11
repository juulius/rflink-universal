"""Verify real shipped YAML API wiring, not a fabricated reference config."""
from pathlib import Path
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
config = yaml.load((root / 'rflink-universal.yaml').read_text(), Loader=Loader)
source = (root / 'rflink_universal/components/rflink_universal/rflink_universal.cpp').read_text()
header = (root / 'rflink_universal/components/rflink_universal/rflink_universal.h').read_text()
html = (root / 'rflink_universal/ui/index.html').read_text()
radio_ids = [r['id'] for r in config['cc1101']]
assert radio_ids == ['cc1101_433_42', 'cc1101_433_92', 'cc1101_868']
assert [c['name'] for c in config['rflink_universal']['channels']] == ['433.42', '433.92', '868.30 Itho']
poll = config['interval'][0]['then'][0]['lambda']
assert all(f'id(rflink_core).run_radio_watchdog({i}, id({r}).is_failed())' in poll for i, r in enumerate(radio_ids))
assert all(f'!id({r}).is_failed() && id(rflink_core).radio_watchdog_healthy({i})' in poll for i, r in enumerate(radio_ids))
for suffix in ('42','92'):
    assert f'!id(rx_433_{suffix}).is_failed()' in poll
    assert f'!id(tx_433_{suffix}).is_failed()' in poll
for i, tx in enumerate(config['remote_transmitter']):
    action = tx['on_transmit']['then'][0]['if']
    assert f'cc1101_433_{"42" if i == 0 else "92"}' in action['condition']['lambda']
    assert any(f'record_tx_command({i},' in item.get('lambda', '') for item in action['then']), '433 TX log must be guarded'
for script in config.get('script', [])[:3]:
    ifs = [item['if'] for item in script['then'] if 'if' in item]
    assert len(ifs) == 1 and 'cc1101_868' in ifs[0]['condition']['lambda']
    guarded = ifs[0]['then']
    assert any('record_tx_command(2,' in item.get('lambda', '') for item in guarded), '868 TX must be logged only inside the radio guard'
    assert any('cc1101.send_packet' in item for item in guarded[-1]['repeat']['then']), '868 packet mode must still transmit'
assert 'channel_index' in header and 'direction{"RX"}' in header
assert 'if (activity_.size() > 32) activity_.resize(32)' in source
assert '"rx_ready\\\":" << bool_json(ch.rx_ready)' in source
assert '"tx_ready\\\":" << bool_json(ch.tx_ready)' in source
assert 'json_escape(a.direction)' in source
assert 'unsigned(a.channel_index)' in source
assert 'json_escape(a.detail)' in source
assert 'noise_suspect' in header and 'noise_frames' in header
assert 'activity_export_json_' in source and '/rflink/api/activity-export' in source
assert '&& !parent_->channel(channel).rx_ready' in source
assert 'data-view="monitor"' in html and 'data-page="monitor"' in html
assert 'renderLearnRadios' in html and 'renderMonitor' in html
assert 'id="monitorLogType"' in html and 'id="monitorCopy"' in html and 'id="monitorExport"' in html
assert "get('/rflink/api/activity-export')" in html
assert "if(selectedChannel<0||$('#listenBtn').disabled)return" in html
print('Radio monitor config: 3-radio YAML, watchdog, noise telemetry, full export endpoint and monitor UI: PASS')
