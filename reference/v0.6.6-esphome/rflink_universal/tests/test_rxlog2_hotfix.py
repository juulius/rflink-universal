"""RXLOG2 static contracts, to catch the two real hardware crash causes."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
core=(root/'components/rflink_universal/rflink_universal.cpp').read_text()
store=(root/'components/rflink_universal/rflink_log_store.cpp').read_text()
ui=(root/'ui/index.html').read_text()
assert 'json_q(summary) + "\\\",\\\"detail\\\":\\\""' in core, 'summary/detail JSON quote regression'
assert 'void RFLinkWebHandler::send_rx_log_chunked_' in core
assert 'httpd_resp_send_chunk(*request' in core
assert 'heap_caps_malloc(scratch_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)' in core
assert 'rx_log_read(cursor, filter, max_rows + 1, ascending, false)' in core
assert 'if (ok) httpd_resp_send_chunk(*request, nullptr, 0)' in core
assert 'rx_log_copy_json(entry.sequence, scratch, scratch_size, written)' in core
assert 'Store::copy_json' in store
assert 'rxLogRequestId' in ui and "$('#rxLogOlder').hidden=!result.has_more" in ui
assert 'rxLogError=e.message' in ui
assert 'if(!result.has_more)break;' in ui
assert 'limit:\'20\'' in ui
print('RXLOG2 static failure regression contracts: PASS')
