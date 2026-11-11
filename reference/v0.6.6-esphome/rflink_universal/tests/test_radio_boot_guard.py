"""Static regression for shared CC1101 startup hardening."""
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
rflink = config['rflink_universal']
assert rflink['radio_cs_pins'] == ['${radio1_csn}', '${radio2_csn}', '${radio3_csn}']
assert rflink['radio_sck_pin'] == '${spi_sck}'
assert rflink['radio_mosi_pin'] == '${spi_mosi}'
assert rflink['radio_miso_pin'] == '${spi_miso}'
assert rflink['radio_startup_delay'] == '200ms'
assert rflink['radio_gdo_pins'] == [
    '${radio1_gdo0}', '${radio1_gdo2}', '${radio2_gdo0}',
    '${radio2_gdo2}', '${radio3_gdo0}', '${radio3_gdo2}'
]
assert rflink['radio_ready_timeout'] == '10ms'
assert rflink['radio_ready_retries'] == 5
assert rflink['radio_post_guard_delay'] == '25ms'
assert config['substitutions']['cc1101_spi_rate_hz'] == '200000'

header = (root / 'rflink_universal/components/rflink_universal/radio_boot_guard.h').read_text()
source = (root / 'rflink_universal/components/rflink_universal/radio_boot_guard.cpp').read_text()
py = (root / 'rflink_universal/components/rflink_universal/__init__.py').read_text()
yaml_text = (root / 'rflink-universal.yaml').read_text()
assert 'get_setup_priority() const override;' in header
assert 'setup_priority::POWER - 1.0f' in source
assert 'this->deselect_all_();' in source
assert 'GPIO_PULLUP_ONLY' in source
assert 'GPIO_PULLDOWN_ONLY' in source
assert 'SCK=LOW+pull-down' in source
assert 'MOSI=LOW+pull-down' in source
assert 'MISO=input+pull-up' in source
assert 'while (static_cast<uint32_t>(micros() - started) < this->ready_timeout_us_)' in source
assert 'gpio_get_level(static_cast<gpio_num_t>(this->miso_pin_)) == 0' in source
assert 'did not release MISO/SO after CSn HIGH' in source
assert 'Shared MISO/SO is LOW with all CSn HIGH' in source
assert 'CHIP_RDYn timeout' in source and 'CHIP_RDYn ready' in source
assert 'Never mark this component failed' in source
assert 'cv.GenerateID(CONF_RADIO_GUARD_ID)' in py
assert 'guard.add_cs_pin' in py
assert 'guard.set_sck_pin' in py and 'guard.set_mosi_pin' in py and 'guard.set_miso_pin' in py
assert 'guard.set_post_guard_delay_ms' in py
# BOOTGUARD aliases existing SPI/CC1101 pins and must never claim them again via
# ESPHome's pin registry; otherwise validation reports "Pin X is used in multiple places".
assert 'cv.ensure_list(_raw_esp32_gpio_number)' in py
assert 'cv.Required(CONF_RADIO_SCK_PIN): _raw_esp32_gpio_number' in py
assert 'cv.Required(CONF_RADIO_MOSI_PIN): _raw_esp32_gpio_number' in py
assert 'cv.Required(CONF_RADIO_MISO_PIN): _raw_esp32_gpio_number' in py
boot_schema = py[py.index('CONFIG_SCHEMA = cv.Schema({'):py.index('async def to_code(config):')]
assert 'internal_gpio_output_pin_schema' not in boot_schema
assert 'internal_gpio_input_pin_schema' not in boot_schema
assert 'cg.gpio_pin_expression' not in py[py.index('async def to_code(config):'): ]
assert 'gpio_set_direction' in source and 'gpio_get_level' in source
# SPI rate must be applied before BUS-priority SPI initialization.
assert 'priority: 1100.0' in yaml_text
assert yaml_text.count('set_data_rate(${cc1101_spi_rate_hz})') == 3
print('CC1101 SPIBOOT2: deterministic pre-SPI pins, pulls, quiet delays, 200 kHz test clock, CHIP_RDYn retries: PASS')

assert 'configure_pre_power_gpio_inputs_' in source
assert 'GPIO_FLOATING' in source
assert 'set_async_gdo0_rx_safe' in source
assert 'set_async_gdo0_tx_data' in source
assert 'radio.add' not in source
assert 'guard.add_gdo_pin' in py
assert 'priority: 599.5' in yaml_text
assert yaml_text.count('set_async_gdo0_rx_safe(') >= 4
assert yaml_text.count('set_async_gdo0_tx_data(') == 2
print('BOARDSAFE1: GPIO14 and 200ms radio power settle / dual GDO transition checks: PASS')
assert config['rflink_universal']['radio_guard_id'] == 'rflink_radio_guard'
