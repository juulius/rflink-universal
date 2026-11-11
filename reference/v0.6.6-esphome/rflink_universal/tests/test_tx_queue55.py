"""Static queue contract. Does not substitute for ESP-IDF firmware or hardware tests."""
from pathlib import Path
p=Path(__file__).resolve().parents[1]/'components/rflink_universal'
c=(p/'rflink_universal.cpp').read_text()
h=(p/'rflink_universal.h').read_text()
assert 'enqueue_ramses_operation(uid,false,error)' in c
assert 'enqueue_ramses_operation(uid,true,error)' in c
assert 'parent_->ramses_pair_start(uid,error)' not in c
assert 'parent_->ramses_pair_test(uid,error)' not in c
assert 'process_ramses_operation_(); // bounded mailbox' in c
assert 'ramses_op_state_.compare_exchange_strong(free,1,std::memory_order_acq_rel)' in c
assert 'ramses_op_state_.store(2,std::memory_order_release)' in c
assert 'ramses_op_state_.store(0,std::memory_order_release)' in c
assert 'uxTaskGetStackHighWaterMark(nullptr)' in c
assert '"queued":true' in c or '\\"queued\\":true' in c
assert 'std::atomic<uint8_t> ramses_op_state_' in h
print('TX Queue v0.5.5 static contract PASS (hardware validation still required)')
