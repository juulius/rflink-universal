from pathlib import Path
import re
p=Path(__file__).resolve().parents[1]
web=(p/'main/rflink_web_ota.cpp').read_text()
net=(p/'main/rflink_mqtt.cpp').read_text()
assert '"/api/mqtt/config"' in web and '"/api/mqtt/status"' in web
assert 'key_ok(req)' in web
assert 'mqtt_save_settings(candidate)' in web
assert 'esp_mqtt_client_publish(client,discovery_topic.c_str()' in net
assert 'esp_mqtt_client_subscribe(client,"homeassistant/status"' in net
assert 'esp_mqtt_client_start(client)' in net
assert (p/'main/idf_component.yml').is_file()
assert 'espressif/mqtt' in (p/'main/idf_component.yml').read_text()
assert 'rflink::mqtt_service_init();' in (p/'main/main.cpp').read_text()
assert not re.search(r'\.uri\s*=\s*"/api/tx"',web)
print('PASS MQTT native integration contracts')
