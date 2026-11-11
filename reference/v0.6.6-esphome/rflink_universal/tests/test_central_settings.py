"""Static contract for CONFIG1 single-location user settings; ESPHome compile remains separate."""
from pathlib import Path
import re
import yaml
ROOT=Path(__file__).resolve().parents[2]
main=(ROOT/'rflink-universal.yaml').read_text()
option=(ROOT/'rflink_universal/options/ha_mqtt.yaml').read_text()
example=(ROOT/'rflink_universal/secrets.example.yaml').read_text()
class Loader(yaml.SafeLoader): pass
def tags(loader, tag, node):
    if isinstance(node,yaml.ScalarNode):return loader.construct_scalar(node)
    if isinstance(node,yaml.SequenceNode):return loader.construct_sequence(node)
    return loader.construct_mapping(node)
Loader.add_multi_constructor('!',tags)
cfg=yaml.load(main,Loader=Loader)
inc=yaml.load(option,Loader=Loader)['mqtt']
s=cfg['substitutions']
assert list(cfg)[:2]==['substitutions','packages']
required={
 'name','friendly_name','board_revision','spi_sck','spi_mosi','spi_miso',
 'radio1_csn','radio1_gdo0','radio1_gdo2', 'radio2_csn','radio2_gdo0','radio2_gdo2',
 'radio3_csn','radio3_gdo0','radio3_gdo2','status_led_gpio',
 'wifi_ssid','wifi_password','wifi_fallback_ssid','wifi_fallback_password',
 'mqtt_broker','mqtt_port','mqtt_username','api_encryption_key','ota_password',
 'log_level','rf_dedupe_window','rf_candidate_threshold'}
assert required<=s.keys(), sorted(required-s.keys())
assert cfg['packages']=={'rgb_status':'rflink_universal/options/rgb_status_ws2812.yaml'}
assert '  # ha_mqtt: !include rflink_universal/options/ha_mqtt.yaml' in main
for key in ('broker','port','username'):
    assert inc[key]=='${mqtt_'+key+'}',(key,inc[key])
assert inc['password']=='rflink_mqtt_password'  # ESPHome !secret tag stub renders key
assert inc['discovery'] is False and inc['reboot_timeout']=='0s'
assert s['mqtt_broker']=='VUL_IP_OF_HOSTNAAM_IN'
assert s['mqtt_port']==1883 and s['mqtt_username']=='rflink'
assert cfg['wifi']=={'ssid':'${wifi_ssid}','password':'${wifi_password}',
                     'ap':{'ssid':'${wifi_fallback_ssid}','password':'${wifi_fallback_password}'}}
assert cfg['api']['encryption']['key']=='${api_encryption_key}'
assert cfg['ota'][0]['password']=='${ota_password}'
assert cfg['logger']['level']=='${log_level}'
assert cfg['rflink_universal']['dedupe_window']=='${rf_dedupe_window}'
assert cfg['rflink_universal']['candidate_threshold']=='${rf_candidate_threshold}'
for secret in ('wifi_ssid_iot','wifi_password_iot','rflink_fallback_password','rflink__encryption_key','rflink__ota_password'):
    assert '!secret '+secret in main
assert '!secret rflink_mqtt_password' in option
assert 'rflink_mqtt_password:' in example
assert not re.search(r'^(rflink_mqtt_broker|rflink_mqtt_user):',example,re.M)
assert 'CONFIG1' in (ROOT/'docs/INSTELLINGEN.md').read_text()
assert 'RFLINK_FIRMWARE_VERSION' in (ROOT/'rflink_universal/tools/maak_release.ps1').read_text()
assert '$releaseVersion = $versionMatch.Groups[1].Value' in (ROOT/'rflink_universal/tools/test_update_release.ps1').read_text(), 'Windows updater test must use current firmware semantic version'
assert 'MQTT-brokeradres ontbreekt' in (ROOT/'rflink_universal/tools/maak_release.ps1').read_text()
print('CONFIG1: grouped labels, mapped uses, secrets boundaries, MQTT opt-in + placeholder guard: PASS')
