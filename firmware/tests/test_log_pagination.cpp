// Executed host test: independent ring retention, full cursor pagination,
// radio/system classification and CSV-equivalent streaming.
#include "rflink_log_store.hpp"
#include <array>
#include <cassert>
#include <cstdint>
#include <limits>
using namespace rflink;
static size_t count_paged(const LogStore& store,LogQuery q,uint64_t snapshot){
  uint64_t before=0,previous=std::numeric_limits<uint64_t>::max();
  size_t total=0;
  for(size_t page=0;page<100;++page){
    LogRecord result[65]{};
    const size_t n=store.page_before(result,65,before,snapshot,q);
    const size_t first=n>64?1:0;
    for(size_t j=n;j>first;--j){
      assert(result[j-1].sequence<previous);
      assert(result[j-1].sequence<=snapshot);
      previous=result[j-1].sequence;
      ++total;
    }
    if(n<=64) return total;
    before=result[first].sequence;
  }
  assert(false && "Cursor must eventually reach oldest retained record");
  return 0;
}
static size_t count_csv(const LogStore& store,LogQuery q,uint64_t snapshot){
  uint64_t after=0;
  size_t total=0;
  for(unsigned round=0;round<200;++round){
    LogRecord result[24]{};
    const auto n=store.page_after(result,24,after,snapshot,q);
    if(!n)return total;
    for(size_t i=0;i<n;++i){
      assert(result[i].sequence>after);
      ++total;
    }
    after=result[n-1].sequence;
  }
  assert(false && "CSV pagination failed to finish");
  return 0;
}
int main(){
  LogStore store;
  assert(store.configure({}));
  for(unsigned id=0;id<3;++id)
    for(unsigned n=0;n<250;++n)
      store.push(LogStream::signal,LogCode::itho,n,static_cast<uint8_t>(id));
  for(unsigned n=0;n<2000;++n)
    store.push(LogStream::noise,LogCode::raw_pulses,n,static_cast<uint8_t>(n%3));
  for(unsigned n=0;n<512;++n)
    store.push(LogStream::system,LogCode::rx_started,n,static_cast<uint8_t>(n%3));
  for(unsigned n=0;n<512;++n)
    store.push(LogStream::web,LogCode::web_page_open,n,kAnyRadio,
      LogSeverity::info,1,0,0,0,3);
  auto state=store.stats();
  assert(state.ready && state.used[0]==250 && state.used[1]==250 && state.used[2]==250);
  assert(state.used[3]==2000 && state.used[4]==512 && state.used[5]==512);
  for(const auto overwritten:state.overwritten)assert(overwritten==0);
  const uint64_t snapshot=store.latest_sequence();
  LogQuery all{};all.include_noise=true;
  assert(count_paged(store,all,snapshot)==3774);
  assert(count_csv(store,all,snapshot)==3774);
  // Filter exactly one radio across its signal + related System events.
  LogQuery radio{};radio.radio_activity=true;radio.radio=1;
  assert(count_paged(store,radio,snapshot)==421);
  assert(count_csv(store,radio,snapshot)==421);
  // Shared noise is not cloned into three radio rings.
  LogQuery noise{};noise.stream=1;noise.include_noise=true;
  assert(count_paged(store,noise,snapshot)==2000);
  LogQuery web{};web.stream=3;
  assert(count_paged(store,web,snapshot)==512);
  // A full noise ring replaces only its own oldest entries, never another
  // frequency's signal buffer or System/Web history.
  for(unsigned n=0;n<12;++n)
    store.push(LogStream::noise,LogCode::raw_pulses,n,1);
  state=store.stats();
  assert(state.used[3]==2000 && state.overwritten[3]==12);
  for(unsigned id: {0u,1u,2u,4u,5u})assert(state.overwritten[id]==0);
  assert(state.used[0]==250 && state.used[1]==250 && state.used[2]==250);
  assert(state.used[4]==512 && state.used[5]==512);
  // Old snapshots exclude newly added events without duplication.
  LogQuery radio_no_noise{};radio_no_noise.radio_activity=true;radio_no_noise.radio=1;
  assert(count_paged(store,radio_no_noise,snapshot)==421);
}
