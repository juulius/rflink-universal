"""Fail closed if optional MQTT becomes mandatory or user credentials leak."""
from pathlib import Path
import re,yaml
root=Path(__file__).resolve().parents[2]
base=(root/'rflink-universal.yaml').read_text()
option=(root/'rflink_universal/options/ha_mqtt.yaml').read_text()
code=(root/'rflink_universal/components/rflink_universal/rflink_universal.cpp').read_text()
header=(root/'rflink_universal/components/rflink_universal/ha_discovery.h').read_text()
assert re.search(r'^  # ha_mqtt: !include rflink_universal/options/ha_mqtt.yaml$',base,re.M)
assert not re.search(r'^  ha_mqtt:',base,re.M), 'Default firmware must work without MQTT credentials'
assert 'ha_node: ${name}' in base
for label in ('mqtt_broker','mqtt_username','mqtt_port'):
    assert label+':' in base and '${'+label+'}' in option,label
assert '!secret rflink_mqtt_password' in option
assert 'rflink_mqtt_password:' in (root/'rflink_universal/secrets.example.yaml').read_text()
config=yaml.safe_load(option.replace('!secret rflink_mqtt_password','placeholder'))
assert config['mqtt']['discovery'] is False
assert 'reboot_timeout: 0s' in option, 'MQTT outage must not reboot API/web/OTA'
assert 'sync_home_assistant();' in option
assert 'bool RFLinkUniversal::set_home_assistant_enabled' in code
assert 'mqtt_connected()' in code and 'home_assistant_enabled' in code
assert 'homeassistant/' in header and 'unique_id' in header and 'event_types' in header
assert 'rolling_code' not in header and 'rf_identities' not in header
assert 'secrets.yaml' not in [p.name for p in root.rglob('secrets.yaml')]
print('HA MQTT config: opt-in, central broker labels, password secret, native API, reconnect/privacy: PASS')
