"""Verify regressions observed on the actual ESP32-S3-N16R8 serial boot log.

Static build-source assertions: do not claim to have tested an actual device.
"""
from pathlib import Path
import sys
import yaml

root = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parents[2]
component = root / 'rflink_universal/components/rflink_universal'
header = (component / 'rflink_universal.h').read_text(encoding='utf-8')
cpp = (component / 'rflink_universal.cpp').read_text(encoding='utf-8')
config = (root / 'rflink-universal.yaml').read_text(encoding='utf-8')

# Defaults to DATA=600 if missing. ESPHome WiFi setup is 250, its own
# web_server uses WIFI - 1.0f. The serial trace shows base->init() before WiFi
# caused lwip's sys_mutex_lock assert and repeated safe-mode boots.
assert 'get_setup_priority() const override { return setup_priority::WIFI - 1.0f; }' in header, 'web server starts before LwIP'
assert 'RFLINK_BOOT_ORDER_FIX_20260918' in cpp, 'missing boot fix marker'
assert 'base->add_handler(this->web_handler_);' in cpp and 'base->init();' in cpp, 'RFLink HTTP handler not correctly registered'

class Loader(yaml.SafeLoader):
    pass

def unknown_tag(loader, suffix, node):
    if isinstance(node, yaml.ScalarNode):
        return loader.construct_scalar(node)
    if isinstance(node, yaml.SequenceNode):
        return loader.construct_sequence(node)
    return loader.construct_mapping(node)

Loader.add_multi_constructor('!', unknown_tag)
data = yaml.load(config, Loader=Loader)
rx = data['remote_receiver']
assert len(rx) == 2, 'expected exactly 2 raw 433MHz receiver channels'
assert all(ch['use_dma'] is False and ch['rmt_symbols'] == 96 for ch in rx), 'ESP32-S3 single RMT RX DMA channel or memory exhaustion regression'
assert len(data['cc1101']) == 3, 'radio inventory changed unintentionally'
assert data['esp32']['variant'] == 'esp32s3' and data['esp32']['flash_size'] == '16MB', 'board configuration changed'
assert data['psram']['mode'] == 'octal', 'N16R8 PSRAM configuration changed'
assert data['rflink_universal']['web_ui'] is True, 'RFLink UI was disabled instead of fixed'
print('Boot order, web handler, 2x96 non-DMA RMT RX, N16R8, 3 optional radios: PASS (static)')
