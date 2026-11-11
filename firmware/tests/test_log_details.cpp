#include "rflink_log_details.hpp"
#include <cassert>
#include <cstring>
#include <memory>
#include <vector>

int main() {
  using namespace rflink;
  auto store=std::make_unique<DetailStore>();
  // 1% makes FIFO testing fast, while preserving six independent arenas.
  assert(store->configure(1));
  auto snapshot=std::make_unique<DetailSnapshot>();
  const auto base=store->stats();
  assert(base.ready);
  assert(base.capacity[0]==512*1024/100);
  assert(base.capacity[3]==448*1024/100);
  assert(base.capacity[4]==384*1024/100);
  assert(base.object_bytes+base.capacity[0]+base.capacity[1]+
         base.capacity[2]+base.capacity[3]+base.capacity[4]+
         base.capacity[5]<DetailStore::kMaxBudgetBytes);

  const char system[]="SPI PARTNUM/CHIP_RDYn readback mismatch";
  const char web[]="HTTP API error 500, redacted";
  assert(store->save_text(4,100,system,sizeof(system)-1));
  assert(store->save_text(5,101,web,sizeof(web)-1));

  std::vector<int32_t> pulses(512);
  for(size_t i=0;i<pulses.size();++i)pulses[i]=(i%2)?-640:320;
  assert(store->save_rf(false,0,111,12345,433420000,pulses.data(),nullptr,pulses.size()));
  assert(store->save_rf(false,1,222,12346,433920000,pulses.data(),nullptr,pulses.size()));
  const auto kept=store->stats();
  // Overflow only shared noise with hundreds of frames: independent 433/System/Web.
  for(uint32_t n=1;n<=300;++n)
    assert(store->save_rf(true,n%3,n,13000+n,433420000,
                          pulses.data(),nullptr,pulses.size()));
  const auto after=store->stats();
  assert(after.evicted[3]>0);
  assert(after.evicted[0]==kept.evicted[0]);
  assert(after.evicted[1]==kept.evicted[1]);
  assert(after.evicted[4]==0);
  assert(after.evicted[5]==0);
  assert(store->copy(0,0,111,snapshot.get()));
  assert(snapshot->format==DetailFormat::pulses);
  assert(store->copy(1,1,222,snapshot.get()));
  assert(store->copy(4,255,100,snapshot.get()));
  assert(snapshot->format==DetailFormat::text);
  assert(snapshot->size==sizeof(system)-1);
  assert(std::memcmp(snapshot->bytes.data(),system,sizeof(system)-1)==0);
  assert(store->copy(5,255,101,snapshot.get()));
  assert(!store->copy(3,1,1,snapshot.get())); // oldest noise evicted

  // Extra info can vary in length and supports exact 63 byte FIFO samples.
  uint8_t fifo[63]{};
  for(size_t i=0;i<sizeof(fifo);++i)fifo[i]=static_cast<uint8_t>(i);
  assert(store->save_rf(false,2,9000,777,868300000,nullptr,fifo,63,true,-179,25,300));
  assert(store->copy(2,2,9000,snapshot.get()));
  assert(snapshot->format==DetailFormat::packet);
  RfDetailMeta meta{};
  std::memcpy(&meta,snapshot->bytes.data(),sizeof(meta));
  assert(meta.count==63 && meta.has_quality==1 && meta.rssi_dbm_x2==-179);
  assert(std::memcmp(snapshot->bytes.data()+sizeof(meta),fifo,63)==0);
  assert(!store->save_text(3,100,"no",2));
  assert(!store->copy(2,1,9000,snapshot.get())); // wrong radio

  // A frame that does not fit must be rejected without removing old records.
  auto before_reject=store->stats();
  assert(!store->save_rf(false,0,500,1,433420000,pulses.data(),nullptr,1025));
  auto after_reject=store->stats();
  assert(before_reject.entries[0]==after_reject.entries[0]);
  assert(store->copy(0,0,111,snapshot.get()));
  return 0;
}
