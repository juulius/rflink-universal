from pathlib import Path
import json
ROOT=Path(__file__).resolve().parents[1]
comp=ROOT/'components/rflink_universal'
cpp=(comp/'rflink_universal.cpp').read_text()
h=(comp/'rflink_universal.h').read_text()
html=(ROOT/'ui/index.html').read_text()
assert 'rx_log_->add(' in cpp
assert 'full_frame_detail_(frame, hash, item_count)' in cpp
assert 'const size_t limit = frame.raw->size();' in cpp
assert 'MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT' in cpp
assert 'rx_log_mutex_' in h and 'xSemaphoreTake(rx_log_mutex_' in cpp
assert '/rflink/api/rx-log-config' in cpp
assert '/rflink/api/rx-log' in cpp
assert 'commit_limits_to_nvs' in cpp
assert 'write_token' in cpp and 'HTTP_POST' in cpp
assert 'rxLogRows.concat(liveTx)' in html
assert 'monitorLogType' in html and 'fetchRxLogPage(' in html
assert 'exportAllRxLogs' in html and 'RFLogTools.zipStore' in html
for lang in 'en nl fr de'.split():
 d=json.loads((ROOT/'ui/locales'/f'{lang}.json').read_text())
 for key in ('rxlog.type','rxlog.noise','rxlog.settings_help','rxlog.save','rxlog.saved','rxlog.exporting'):
  assert d.get(key), (lang,key)
print('RX log integration contracts: PASS')
