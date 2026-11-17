// Host-only deterministic and concurrent tests of ISR-to-worker handoff.
// No CC1101 register writes, GPIO drivers, ESP32 interrupt, or live RF.
#include "rflink_sync_edge_ring.hpp"
#include "rflink_clocked_bit_capture.hpp"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <thread>

using namespace rflink;

static void test_full_queue_loss_markers() {
  auto q=std::make_unique<SyncClockEdgeRing>();
  assert(q->pending()==0);
  for(uint32_t i=0;i<SyncClockEdgeRing::kCapacity;++i)
    assert(q->push_isr((i&1u)!=0,100000+i*26u));
  assert(q->pending()==SyncClockEdgeRing::kCapacity);
  assert(!q->push_isr(true,900000));
  assert(!q->push_isr(true,900026));
  assert(!q->push_isr(true,900052));
  assert(q->dropped_total()==3);
  SyncClockEdge first{};
  assert(q->pop(first) && first.timestamp_us==100000 &&
         first.lost_before==0 && !first.data_high);
  assert(q->pop(first) && first.timestamp_us==100026 &&
         first.lost_before==0 && first.data_high);
  assert(q->push_isr(true,900078));
  uint32_t count=0;
  uint64_t previous=0;
  SyncClockEdge edge{};
  while(q->pop(edge)) {
    assert(edge.timestamp_us>previous);
    previous=edge.timestamp_us;
    if(edge.lost_before) {
      assert(edge.timestamp_us==900078 && edge.lost_before==3);
      ++count;
    }
  }
  assert(count==1);
  assert(previous==900078 && q->pending()==0);
  assert(q->trailing_loss_after_stop()==0);
  // After producer has been quiesced, old epoch data must be cleared without
  // changing cumulative failure evidence.
  assert(q->push_isr(false,999999));
  q->reset_epoch();
  assert(q->pending()==0 && q->dropped_total()==3);
}

static void test_last_loss_without_next_edge() {
  auto q=std::make_unique<SyncClockEdgeRing>();
  for(uint32_t i=0;i<SyncClockEdgeRing::kCapacity;++i)
    assert(q->push_isr(false,i*26u));
  for(unsigned i=0;i<7;++i) assert(!q->push_isr(true,100000+i*26u));
  SyncClockEdge edge{};
  unsigned count=0;
  while(q->pop(edge))++count;
  assert(count==SyncClockEdgeRing::kCapacity);
  assert(q->trailing_loss_after_stop()==7);
  assert(q->trailing_loss_after_stop()==0);
}

static void test_worker_epoch_and_real_discontinuity() {
  auto q=std::make_unique<SyncClockEdgeRing>();
  ClockedBitCapture capture;
  assert(capture.begin_epoch(40));
  for(uint32_t i=0;i<SyncClockEdgeRing::kCapacity;++i)
    assert(q->push_isr((i&1u)!=0,1000+i*26u));
  for(unsigned i=0;i<5;++i) assert(!q->push_isr(true,55000+i*26u));

  SyncClockEdge edge{};
  assert(q->pop(edge));
  assert(capture.append(edge.data_high,edge.timestamp_us,40));
  assert(q->push_isr(true,56000)); // marker must survive worker handoff
  while(q->pop(edge)) {
    if(edge.lost_before) capture.report_lost_edges(edge.lost_before);
    assert(capture.append(edge.data_high,edge.timestamp_us,40));
  }
  capture.end_window();
  ClockedBitBlock block{};
  assert(capture.pop(block));
  assert(block.bit_count==2048 && !block.discontinuity_before);
  assert(capture.pop(block));
  assert(block.discontinuity_before && block.bit_count==1);
  assert(capture.stats().reported_missing_edges==5);
  assert(!capture.pop(block));
}

static void test_spsc_no_overwrite_stress() {
  constexpr uint32_t kCount=75000;
  auto q=std::make_unique<SyncClockEdgeRing>();
  std::atomic<bool> done{false};
  uint32_t received=0;
  uint64_t last_timestamp=0;
  uint64_t reported_loss=0;

  std::thread producer([&]{
    for(uint32_t i=1;i<=kCount;++i)
      (void)q->push_isr((i&1u)!=0,uint64_t(i)*26u);
    done.store(true,std::memory_order_release);
  });
  std::thread consumer([&]{
    SyncClockEdge sample{};
    while(!done.load(std::memory_order_acquire) || q->pending()!=0) {
      if(!q->pop(sample)) {std::this_thread::yield();continue;}
      assert(sample.timestamp_us>last_timestamp);
      last_timestamp=sample.timestamp_us;
      reported_loss+=sample.lost_before;
      ++received;
    }
  });
  producer.join();
  consumer.join();
  reported_loss+=q->trailing_loss_after_stop();
  assert(uint64_t(received)+q->dropped_total()==kCount);
  assert(reported_loss==q->dropped_total());
}
int main(){
  test_full_queue_loss_markers();
  test_last_loss_without_next_edge();
  test_worker_epoch_and_real_discontinuity();
  test_spsc_no_overwrite_stress();
  std::puts("PASS 868 synchronous GPIO clock-edge ring: SPSC, loss, recovery, epochs");
}
