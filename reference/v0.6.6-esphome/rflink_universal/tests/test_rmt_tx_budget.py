"""Test delivered YAML/packages against ESP32-S3 RMT allocation regression.

Static allocation test only: the actual ESP-IDF driver, on-board WS2812,
attached CC1101 radios, and OTA are hardware/integration tests.

Usage: python test_rmt_tx_budget.py [path/to/project/root]
"""
from pathlib import Path
import sys
import yaml

ROOT = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path(__file__).resolve().parents[2]

class EspHomeLoader(yaml.SafeLoader):
    pass

def tag(loader, _suffix, node):
    if isinstance(node, yaml.ScalarNode):
        return loader.construct_scalar(node)
    if isinstance(node, yaml.SequenceNode):
        return loader.construct_sequence(node)
    return loader.construct_mapping(node)

EspHomeLoader.add_multi_constructor('!', tag)
config = yaml.load((ROOT / 'rflink-universal.yaml').read_text(encoding='utf-8'), Loader=EspHomeLoader)
package = yaml.load((ROOT / 'rflink_universal/options/rgb_status_ws2812.yaml').read_text(encoding='utf-8'), Loader=EspHomeLoader)

assert config['esp32']['variant'] == 'esp32s3'
assert config['esp32']['flash_size'] == '16MB'
assert config['psram']['mode'] == 'octal'
assert config['packages']['rgb_status'] == 'rflink_universal/options/rgb_status_ws2812.yaml'
assert config['substitutions']['status_led_gpio'] == 'GPIO48'

radios = {radio['id']: radio for radio in config['cc1101']}
assert set(radios) == {'cc1101_433_42', 'cc1101_433_92', 'cc1101_868'}, 'All three CC1101 radios must survive the RMT fix'
assert radios['cc1101_868']['packet_mode'] is True, 'Itho 868 must remain packet-mode SPI'
assert radios['cc1101_868']['modulation_type'] == '2-FSK', 'Itho mode changed'
assert config['substitutions'][radios['cc1101_868']['gdo0_pin'][2:-1]] == 'GPIO16'
assert radios['cc1101_868']['frequency'] == '868.2999MHz'
assert 'rmt_symbols' not in radios['cc1101_868'] and 'use_dma' not in radios['cc1101_868'], '868 SPI packet mode must not reserve RMT'
assert len(config['remote_receiver']) == 2
assert len(config['remote_transmitter']) == 2

rx = {r['id']: r for r in config['remote_receiver']}
tx = {t['id']: t for t in config['remote_transmitter']}
assert set(rx) == {'rx_433_42', 'rx_433_92'}
assert set(tx) == {'tx_433_42', 'tx_433_92'}
assert [config['substitutions'][rx[n]['pin']['number'][2:-1]] for n in ('rx_433_42', 'rx_433_92')] == ['GPIO6', 'GPIO14']
assert [config['substitutions'][tx[n]['pin'][2:-1]] for n in ('tx_433_42', 'tx_433_92')] == ['GPIO5', 'GPIO8']
for label, table, expected in (('RX', rx, 96), ('TX', tx, 48)):
    for identifier, item in table.items():
        assert item.get('use_dma') is False, f'{label} {identifier}: DMA must be disabled'
        assert item.get('rmt_symbols') == expected, f'{label} {identifier}: expected {expected} RMT symbols'
        assert item['rmt_symbols'] % 48 == 0, 'S3 memory allocation must be a multiple of 48'

leds = package['light']
assert len(leds) == 1
led = leds[0]
assert led['platform'] == 'esp32_rmt_led_strip'
assert led['id'] == 'rflink_status_rgb'
assert led['num_leds'] == 1 and led['pin'] == '${status_led_gpio}'
assert led.get('use_dma') is False and led.get('rmt_symbols') == 48, 'LED must reserve one 48-symbol non-DMA TX block'
assert led['chipset'].upper() == 'WS2812'
assert led['channel_colors'] == 'GRB'

rx_total = sum(x['rmt_symbols'] for x in rx.values())
tx_total = sum(x['rmt_symbols'] for x in tx.values()) + led['rmt_symbols']
assert rx_total == 192 and tx_total == 144, 'ESP32-S3 total RMT budget changed unexpectedly'
assert rx_total <= 192 and tx_total <= 192 and 192 - tx_total == 48
assert all(item['carrier_duty_percent'] == '100%' for item in tx.values())
assert all(item['non_blocking'] is True for item in tx.values())
assert all('on_transmit' in item and 'on_complete' in item for item in tx.values())

print('PASS: RX 2x96=192; TX 2x48 + LED48 =144/192; GPIO48; all 3 CC1101 incl. 868 SPI packet; no DMA (STATIC ONLY)')
