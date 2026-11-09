from pathlib import Path
root=Path(__file__).resolve().parents[1]
code=(root/'main'/'rflink_web_ota.cpp').read_text()
# Two-slot N16R8 board must select the inactive partition explicitly.
# A one-slot live table makes the IDF round-robin helper return the
# running partition, which esp_ota_begin correctly refuses to overwrite.
assert 'inspect_ota_layout()' in code
assert 'choose_native_ota_slot' in code
assert 'const esp_partition_t* next=ota_layout.target;' in code
assert 'if(!ota_layout.plan.safe || !ota_layout.target)' in code
assert 'esp_ota_begin' in code and 'esp_ota_write' in code
assert 'esp_ota_end' in code and 'esp_ota_set_boot_partition' in code
assert 'X-RFLink-Update-Key' in code
assert 'WIFI_AUTH_WPA2_PSK' in code
assert 'nvs_set_str' in code
p=(root/'partitions.csv').read_text()
assert 'ota_0' in p and 'ota_1' in p and 'otadata' in p
print('Web/OTA source contract: OK (not firmware compilation)')
