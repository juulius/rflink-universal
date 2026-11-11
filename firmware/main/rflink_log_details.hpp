#pragma once
// Bounded volatile detail evidence. Six physically independent FIFO byte arenas:
// 433.42, 433.92, 868.30, shared noise, System and Web.
// This is an OPTIONAL companion to LogStore; it never changes log limits/NVS/CSV.
// Construct this object AND its byte arenas in PSRAM on an ESP32-S3.
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <mutex>

namespace rflink {
enum class DetailFormat : uint8_t { none=0, pulses=1, packet=2, text=3 };
struct RfDetailMeta {
  uint32_t uptime_ms=0;
  uint32_t frequency_hz=0;
  uint32_t hash=0;
  int32_t frequency_offset_hz=0;
  uint16_t count=0;
  int16_t rssi_dbm_x2=0;
  uint8_t lqi=0;
  uint8_t has_quality=0;
};
constexpr size_t kMaxDetailData=sizeof(RfDetailMeta)+1024*sizeof(int32_t);
struct DetailSnapshot {
  DetailFormat format=DetailFormat::none;
  uint16_t size=0;
  std::array<uint8_t,kMaxDetailData> bytes{};
};
struct DetailStats {
  bool ready=false;
  size_t object_bytes=0;
  std::array<size_t,6> capacity{};
  std::array<size_t,6> used{};
  std::array<size_t,6> entries{};
  std::array<uint64_t,6> evicted{};
  std::array<uint64_t,6> rejected{};
};
class DetailStore {
 public:
  using Allocator=void*(*)(size_t,size_t);
  using Deallocator=void(*)(void*);
  // Arena sum 2368 KiB + a <=64 KiB index/object = <=2432 KiB,
  // below 30% of 8 MiB PSRAM. Existing compact logs are separate.
  static constexpr size_t kMaxBudgetBytes=2432*1024;
  static constexpr std::array<size_t,6> kArenaKiB={{512,512,256,448,384,256}};
  static constexpr std::array<size_t,6> kEntryLimits={{250,250,250,2000,512,512}};
  static constexpr size_t kTotalEntries=3774;

  explicit DetailStore(Allocator allocate=&std::calloc,Deallocator release=&std::free)
    :allocate_(allocate),release_(release) {
    size_t offset=0;
    for(size_t i=0;i<rings_.size();++i) {
      rings_[i].entries=index_.data()+offset;
      rings_[i].max_entries=kEntryLimits[i];
      offset+=kEntryLimits[i];
    }
  }
  DetailStore(const DetailStore&)=delete;
  DetailStore& operator=(const DetailStore&)=delete;
  ~DetailStore(){clear();}
  // Startup only. A 50% attempt is permitted if the full allocation fails.
  // Never fall back to internal SRAM via the embedded allocator.
  bool configure(unsigned percent=100) {
    if(ready_ || percent==0 || percent>100)return false;
    clear();
    for(size_t i=0;i<rings_.size();++i) {
      auto& ring=rings_[i];
      ring.capacity=kArenaKiB[i]*1024*percent/100;
      ring.bytes=static_cast<uint8_t*>(allocate_(1,ring.capacity));
      if(!ring.bytes){clear();return false;}
    }
    ready_=true;
    return true;
  }
  bool ready() const {return ready_;}
  // Decoder event sequence is the key for radio; LogStore sequence for System/Web.
  // RF frames are binary, exact and uncompressed. No heap allocation on RX.
  bool save_rf(bool noise,uint8_t radio,uint64_t key,uint32_t uptime_ms,
               uint32_t frequency_hz,const int32_t* pulses,
               const uint8_t* packet,size_t count,bool has_quality=false,
               int16_t rssi_dbm_x2=0,uint8_t lqi=0,
               int32_t frequency_offset_hz=0) {
    if(!ready_ || radio>=3 || key==0 ||
       (!!pulses==!!packet) ||
       (pulses && (count<2 || count>1024)) ||
       (packet && (count<1 || count>63)))return false;
    RfDetailMeta meta{};
    meta.uptime_ms=uptime_ms;meta.frequency_hz=frequency_hz;
    meta.count=static_cast<uint16_t>(count);
    meta.has_quality=has_quality?1:0;
    meta.rssi_dbm_x2=rssi_dbm_x2;meta.lqi=lqi;
    meta.frequency_offset_hz=frequency_offset_hz;
    uint32_t hash=2166136261u;
    for(size_t i=0;i<count;++i) {
      if(pulses){
        const int64_t value=pulses[i];
        const uint32_t width=static_cast<uint32_t>(value<0?-value:value);
        const uint32_t bucket=((width+10u)/20u)*20u;
        hash^=bucket^(value<0?0x80000000u:0u);
      } else hash^=packet[i];
      hash*=16777619u;
    }
    meta.hash=hash;
    const auto* data=pulses?static_cast<const void*>(pulses):
                              static_cast<const void*>(packet);
    const size_t size=count*(pulses?sizeof(int32_t):sizeof(uint8_t));
    return save(noise?3:radio,radio,key,pulses?DetailFormat::pulses:DetailFormat::packet,
                &meta,sizeof(meta),data,size);
  }
  // Only vetted diagnostic text; never arbitrary URL/body/secret/keys.
  // System/Web use radio 255. TX diagnostics may use radio bucket 0..2.
  bool save_text(uint8_t bucket,uint8_t radio,uint64_t key,
                 const char* message,size_t bytes) {
    if(!ready_ || bucket>=rings_.size() || !key || !message ||
       bytes==0 || bytes>512 || (bucket<4 ? radio>=3 : radio!=255))
      return false;
    return save(bucket,radio,key,DetailFormat::text,message,bytes,nullptr,0);
  }
  bool save_text(uint8_t bucket,uint64_t key,const char* message,size_t bytes) {
    return save_text(bucket,255,key,message,bytes);
  }
  bool copy(uint8_t bucket,uint8_t radio,uint64_t key,DetailSnapshot* out) const {
    if(!ready_ || bucket>=rings_.size() || !key || !out)return false;
    std::lock_guard<std::mutex> guard(lock_);
    const auto& r=rings_[bucket];
    for(size_t j=0;j<r.count;++j) {
      const size_t ix=(r.oldest+r.count-1-j)%r.max_entries;
      const auto& e=r.entries[ix];
      if(e.key!=key || e.radio!=radio)continue;
      if(e.length>out->bytes.size())return false;
      out->format=e.format;out->size=e.length;
      read_bytes(r,e.offset,out->bytes.data(),e.length);
      return true;
    }
    return false;
  }
  DetailStats stats() const {
    std::lock_guard<std::mutex> guard(lock_);
    DetailStats s{};s.ready=ready_;s.object_bytes=sizeof(*this);
    for(size_t i=0;i<rings_.size();++i) {
      const auto& r=rings_[i];
      s.capacity[i]=r.capacity;s.used[i]=r.used;s.entries[i]=r.count;
      s.evicted[i]=r.evicted;s.rejected[i]=r.rejected;
    }
    return s;
  }
 private:
  struct Index {
    uint64_t key=0;
    uint32_t offset=0;
    uint16_t length=0;
    uint8_t radio=255;
    DetailFormat format=DetailFormat::none;
  };
  struct Ring {
    uint8_t* bytes=nullptr;
    Index* entries=nullptr;
    size_t max_entries=0,capacity=0,used=0,write=0,oldest=0,count=0;
    uint64_t evicted=0,rejected=0;
  };
  void clear(){
    for(auto& r:rings_) {
      if(r.bytes)release_(r.bytes);
      r.bytes=nullptr;r.capacity=r.used=r.write=r.oldest=r.count=0;
      r.evicted=r.rejected=0;
    }
    ready_=false;
  }
  static void write_bytes(Ring& r,const void* source,size_t size) {
    if(!size)return;
    const size_t first=size<r.capacity-r.write?size:r.capacity-r.write;
    std::memcpy(r.bytes+r.write,source,first);
    if(size>first)std::memcpy(r.bytes,static_cast<const uint8_t*>(source)+first,size-first);
    r.write=(r.write+size)%r.capacity;
  }
  static void read_bytes(const Ring& r,size_t offset,void* target,size_t size) {
    const size_t first=size<r.capacity-offset?size:r.capacity-offset;
    std::memcpy(target,r.bytes+offset,first);
    if(size>first)std::memcpy(static_cast<uint8_t*>(target)+first,r.bytes,size-first);
  }
  bool save(uint8_t bucket,uint8_t radio,uint64_t key,DetailFormat format,
            const void* a,size_t a_size,const void* b,size_t b_size) {
    std::lock_guard<std::mutex> guard(lock_);
    auto& r=rings_[bucket];
    const size_t bytes=a_size+b_size;
    if(!r.bytes || bytes>r.capacity || bytes>UINT16_MAX ||
       bytes>kMaxDetailData) {++r.rejected;return false;}
    // Evict only this category's oldest details. All other rings are untouched.
    while(r.count && (r.count>=r.max_entries || r.used+bytes>r.capacity)) {
      const auto& old=r.entries[r.oldest];
      r.used-=old.length;
      r.oldest=(r.oldest+1)%r.max_entries;--r.count;++r.evicted;
    }
    const size_t ix=(r.oldest+r.count)%r.max_entries;
    r.entries[ix]={key,static_cast<uint32_t>(r.write),
                  static_cast<uint16_t>(bytes),radio,format};
    write_bytes(r,a,a_size);
    if(b_size)write_bytes(r,b,b_size);
    r.used+=bytes;++r.count;
    return true;
  }
  Allocator allocate_;
  Deallocator release_;
  mutable std::mutex lock_;
  std::array<Ring,6> rings_{};
  std::array<Index,kTotalEntries> index_{};
};
static_assert(sizeof(DetailStore)<=64*1024,"Detail index exceeds reserved overhead");
static_assert(sizeof(DetailStore)+2368*1024<=DetailStore::kMaxBudgetBytes,
              "Log details must not exceed 30% PSRAM budget");
} // namespace rflink
