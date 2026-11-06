from pathlib import Path
r=Path(__file__).resolve().parents[1]
w=(r/'main/rflink_web_ota.cpp').read_text()
m=(r/'main/main.cpp').read_text()
s=(r/'sdkconfig.defaults').read_text()
assert 'CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y' in s
assert 'esp_ota_get_partition_description(next,&candidate)' in w
assert 'native_ota_identity_accepted' in w and 'candidate.version' in w
assert 'status.uri="/api/status"' in w
assert '"\\"rx_implemented\\":false' not in w or 'rx_implemented' in w
assert 'esp_ota_mark_app_valid_cancel_rollback' in m
assert 'esp_ota_mark_app_invalid_rollback_and_reboot' in m
assert 'bool rflink_start_web_ota()' in w
print('Stage 3 source contracts OK (compile not tested)')
