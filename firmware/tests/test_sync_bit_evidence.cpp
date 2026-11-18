// Pure C++ bounded 868 MHz bit evidence acceptance.
// No physical RF, SPI, secrets or automatic RX transport changes.
#include "rflink_sync_bit_evidence.hpp"
#include <cassert>
#include <cstdio>
#include <memory>
using namespace rflink;
static ClockedBitBlock block(uint32_t epoch, uint16_t n, bool gap=false,
                             bool end=false) {
  ClockedBitBlock b{};
  b.epoch=epoch;b.bit_count=n;b.first_sample_us=1000;b.last_sample_us=2000;
  b.discontinuity_before=gap;b.window_end=end;
  b.bytes[0]=0xA5;b.bytes[1]=0x40;
  return b;
}
static void test_arm_freeze_copy_and_bounded_oldest_retention() {
  SyncBitEvidence evidence;
  ClockedBitBlock copied{};
  assert(!evidence.arm() && !evidence.freeze());
  assert(!evidence.copy(0,&copied));
  std::unique_ptr<ClockedBitBlock[]> store(new ClockedBitBlock[3]{});
  assert(evidence.attach(store.get(),3));
  assert(evidence.stats().ready && evidence.stats().capacity==3);
  assert(evidence.arm() && evidence.stats().recording);
  const auto first=block(7,9);
  const auto second=block(7,2048,true);
  const auto third=block(8,17,false,true);
  assert(evidence.append(first));
  assert(!evidence.copy(0,&copied)); // must explicitly freeze
  assert(evidence.append(second));
  assert(evidence.stats().recorded_discontinuities==1);
  assert(evidence.append(third)); // fills and freezes, no overwrite
  assert(!evidence.stats().recording && evidence.stats().full);
  assert(evidence.stats().recorded_bits==2074);
  assert(evidence.stats().used==3);
  assert(!evidence.append(block(8,12)));
  assert(evidence.stats().discarded_newest==1);
  assert(evidence.copy(0,&copied));
  assert(copied.epoch==7 && copied.bit_count==9 && copied.bytes[0]==0xA5);
  assert(evidence.copy(1,&copied));
  assert(copied.epoch==7 && copied.discontinuity_before);
  assert(evidence.copy(2,&copied));
  assert(copied.epoch==8 && copied.window_end);
  assert(!evidence.copy(3,&copied));
  assert(!evidence.freeze()); // already automatically frozen
  const uint32_t generation=evidence.stats().generation;
  assert(evidence.arm());
  assert(evidence.stats().generation==generation+1);
  assert(evidence.stats().used==0 && !evidence.stats().full);
  assert(evidence.append(block(9,1,true,true)));
  assert(evidence.freeze());
  assert(!evidence.stats().recording);
  assert(evidence.copy(0,&copied));
  assert(copied.epoch==9 && copied.discontinuity_before);
  assert(evidence.stats().used==1);
  assert(!evidence.append(block(10,4)));
}
static void test_validation_and_preservation_on_rejected_blocks() {
  SyncBitEvidence e;
  ClockedBitBlock storage[2]{};
  assert(!e.attach(nullptr,2));
  assert(!e.attach(storage,0));
  assert(e.attach(storage,2));
  assert(e.arm());
  auto malformed=block(0,24);
  assert(!e.append(malformed));
  malformed=block(2,0);
  assert(!e.append(malformed));
  malformed=block(2,2049);
  assert(!e.append(malformed));
  assert(e.stats().used==0 && e.stats().recording);
  assert(e.append(block(3,8)));
  assert(e.freeze());
  assert(e.stats().used==1 && e.stats().recorded_bits==8);
  assert(e.copy(0,&malformed) && malformed.epoch==3);
}
int main() {
  test_arm_freeze_copy_and_bounded_oldest_retention();
  test_validation_and_preservation_on_rejected_blocks();
  std::puts("PASS bounded 868 sync-bit evidence: arming, freezing, epochs, loss and no overwrite");
}
