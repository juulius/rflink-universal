from pathlib import Path
import subprocess
ROOT=Path(__file__).resolve().parents[1]
CONFIG=(ROOT/'sdkconfig.defaults').read_text()
MAIN=(ROOT/'main/main.cpp').read_text()
WEB=(ROOT/'main/rflink_web_ota.cpp').read_text()
RMT=(ROOT/'components/rflink_radio_hal/rflink_rmt_rx.cpp').read_text()
HEADER=(ROOT/'components/rflink_radio_hal/include/rflink_rmt_rx.hpp').read_text()
assert 'CONFIG_SPIRAM_MODE_OCT=y' in CONFIG
assert 'CONFIG_ESPTOOLPY_FLASHMODE_QIO=y' in CONFIG
assert 'vTaskDelay(pdMS_TO_TICKS(10000))' in MAIN
assert 'rx_worker_started' in MAIN
assert 'esp_ota_mark_app_invalid_rollback_and_reboot()' in MAIN
assert 'ota_busy' in WEB and 'esp_ota_get_partition_description' in WEB
assert 'std::lock_guard<std::mutex> g(stats_mutex_)' in RMT
assert 'mutable std::mutex stats_mutex_;' in HEADER
subprocess.run(['python3',str(ROOT/'tools/preflight.py')],check=True)
# Packaging must fail closed without actual IDF build files.
run=subprocess.run(['python3',str(ROOT/'tools/package_release.py')],stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
assert run.returncode != 0 and 'Missing official IDF project_description.json' in run.stderr,run.stderr
print('PASS Stage13: board configs, delayed OTA health, RMT stats concurrency, missing-IDF-build release fail-closed')

