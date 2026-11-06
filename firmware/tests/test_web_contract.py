from pathlib import Path
root=Path(__file__).resolve().parents[1]
code=(root/'main'/'rflink_web_ota.cpp').read_text()
assert 'esp_ota_get_next_update_partition' in code
assert 'esp_ota_begin' in code and 'esp_ota_write' in code
assert 'esp_ota_end' in code and 'esp_ota_set_boot_partition' in code
assert 'X-RFLink-Update-Key' in code
assert 'WIFI_AUTH_WPA2_PSK' in code
assert 'nvs_set_str' in code
p=(root/'partitions.csv').read_text()
assert 'ota_0' in p and 'ota_1' in p and 'otadata' in p
print('Web/OTA source contract: OK (not firmware compilation)')
