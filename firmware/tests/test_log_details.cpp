#include "rflink_log_details.hpp"
#include <cassert>
#include <cstring>
#include <memory>
#include <vector>

int main() {
  using namespace rflink;
  auto store=std::make_unique<DetailStore>();
  // 2% makes FIFO tests fast. The 192-KiB short-noise arena must still
  // fit one 512-duration pulse train, even in this scaled-down host test.
  assert(store->configure(2));
  auto snapshot=std::make_unique<DetailSnapshot>();
  const auto base=store->stats();
  assert(base.ready);
  assert(base.capacity[0]==512*1024*2/100);
  assert(base.capacity[3]==192*1024*2/100);
  assert(base.capacity[6]==256*1024*2/100);
  assert(base.capacity[4]==384*1024*2/100);
  assert(base.object_bytes+base.capacity[0]+base.capacity[1]+
         base.capacity[2]+base.capacity[3]+base.capacity[4]+
         base.capacity[5]+base.capacity[6]<DetailStore::kMaxBudgetBytes);

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

  // Unknown 868 FIFO packets MUST NOT evict recognized 868 or 433 noise details.
  const uint64_t k868Unknown=(uint64_t(1)<<63)|9010;
  assert(store->save_rf(true,2,k868Unknown,800,868300000,nullptr,fifo,63,true,-160,33,-200));
  assert(store->copy(DetailStore::kUnknown868DetailBucket,2,k868Unknown,snapshot.get()));
  assert(snapshot->format==DetailFormat::packet);
  assert(std::memcmp(snapshot->bytes.data()+sizeof(meta),fifo,63)==0);
  assert(!store->copy(3,2,k868Unknown,snapshot.get()));
  const auto before_868_storm=store->stats();
  for(uint32_t n=1;n<240;++n)
    assert(store->save_rf(true,2,(uint64_t(1)<<63)|(9010+n),800+n,
                          868300000,nullptr,fifo,63,true,-160,33,-200));
  const auto after_868_storm=store->stats();
  assert(after_868_storm.evicted[6]>before_868_storm.evicted[6]);
  assert(after_868_storm.evicted[3]==before_868_storm.evicted[3]);
  assert(after_868_storm.evicted[2]==before_868_storm.evicted[2]);
  assert(store->copy(2,2,9000,snapshot.get())); // recognized frame survives
  assert(!store->copy(6,2,k868Unknown,snapshot.get())); // own old unknown evicted
  // A pre-filter RMT completion may contain only one valid signed duration.
  const int32_t one_pulse=120;
  const uint64_t pre_key=(uint64_t(1)<<63)|(uint64_t(1)<<62)|112;
  assert(store->save_rf(false,1,pre_key,12400,433920000,&one_pulse,nullptr,1));
  assert(store->copy(1,1,pre_key,snapshot.get()));
  RfDetailMeta sample_meta{};
  std::memcpy(&sample_meta,snapshot->bytes.data(),sizeof(sample_meta));
  assert(sample_meta.count==1 && snapshot->format==DetailFormat::pulses);
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
