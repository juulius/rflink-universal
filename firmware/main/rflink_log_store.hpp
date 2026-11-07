#pragma once
// RFLink v0.7 log store: bounded fixed records, no payloads, strings or
// allocation on the RF receive path. Every ring has its own retention count.
// The single noise ring is shared across all three CC1101 radios.
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <mutex>

namespace rflink {
enum class LogStream : uint8_t { signal=0, noise=1, system=2, web=3 };
enum class LogSeverity : uint8_t { info=0, warning=1, error=2 };
enum class LogCode : uint16_t {
  raw_pulses=1, unknown_packet=2, itho=3, ramses=4,
  boot=10, radio_missing=11, rx_recovered=12, rx_restart_failure=13,
  wifi_connected=14, wifi_disconnected=15, setup_started=16,
  settings_saved=20, settings_rejected=21, ota_attempt=22, ota_rejected=23,
  tx_rejected=30, tx_started=31, tx_driver_complete=32,
  tx_failed=33, tx_restore_failed=34
};
constexpr uint8_t kAnyRadio=255;
constexpr size_t kSignalRadios=3;
constexpr size_t kLogRings=6;
constexpr size_t kSystemLogRetention=128;
constexpr size_t kWebLogRetention=128;
constexpr size_t kLogBudgetBytes=1536*1024;

constexpr uint32_t kLogSettingsMagic=0x52464C47; // RFLG
struct LogLimits {
  uint16_t signal[3]={250,250,250};
  uint16_t noise=2000;
};

struct SavedLogLimits {
  uint32_t magic=kLogSettingsMagic;
  uint16_t version=1;
  uint16_t reserved=0;
  LogLimits limits{};
};

struct LogRecord {
  uint64_t sequence=0;       // Correlation with other records in this boot.
  uint32_t uptime_ms=0;      // Monotonic uptime, not an invented wall-clock time.
  uint32_t correlation_id=0; // Zero when no related operation is known.
  int32_t value=0;           // Sanitized numeric error/status code only.
  uint16_t code=0;           // Enum-backed; never a user-supplied string.
  uint16_t count=0;          // Frame/pulse count, not captured RF data.
  uint8_t stream=0;
  uint8_t radio=kAnyRadio;
  uint8_t severity=0;
  uint8_t flags=0;          // TX: bit0 RX active, bit1 RX restored, bit2 driver called,
                            // bit3 driver accepted, bit4 physical hardware restored.
  uint16_t profile=0;       // TX RF profile; never a paired device ID.
  uint8_t transport=255;    // TX transport (UART/RMT/FIFO), 255 if N/A.
  uint8_t tx_stage=0;       // Failure stage; none if N/A.
};
static_assert(sizeof(LogRecord)<=40,"Unexpected log record size");

struct LogStats {
  LogLimits limits{};
  size_t allocated_bytes=0;
  uint32_t used[6]={};
  uint64_t overwritten[6]={};
  bool ready=false;
};
struct LogQuery {
  int stream=-1;            // -1 = all; 0/1/2/3 matches LogStream.
  int radio=-1;             // -1 = all; 0/1/2 exact radio.
  bool include_noise=false;
};

class LogStore {
 public:
  using Allocator=void*(*)(size_t,size_t);
  using Deallocator=void(*)(void*);
  explicit LogStore(Allocator allocate=&std::calloc,
                    Deallocator release=&std::free)
      :allocate_(allocate),release_(release){}
  LogStore(const LogStore&)=delete;
  LogStore& operator=(const LogStore&)=delete;
  ~LogStore() {
    for(auto& ring:rings_)release_(ring.data);
  }

  static size_t bytes_for(const LogLimits& limits) {
    return (size_t(limits.signal[0])+limits.signal[1]+limits.signal[2]+
            limits.noise+kSystemLogRetention+kWebLogRetention)*sizeof(LogRecord);
  }
  static bool valid_limits(const LogLimits& limits) {
    return limits.signal[0]<=4000 && limits.signal[1]<=4000 &&
           limits.signal[2]<=4000 && limits.noise<=10000 &&
           bytes_for(limits)<=kLogBudgetBytes;
  }

  // All-or-nothing resize, preserving the newest records and overwritten
  // counters. Any allocator failure leaves all six previous rings untouched.
  bool configure(const LogLimits& limits) {
    if(!valid_limits(limits))return false;
    std::lock_guard<std::mutex> lock(mutex_);
    const size_t requested[6]={limits.signal[0],limits.signal[1],
      limits.signal[2],limits.noise,kSystemLogRetention,kWebLogRetention};
    std::array<Ring,6> next{};
    for(size_t i=0;i<next.size();++i) {
      next[i].capacity=requested[i];
      if(requested[i]) {
        next[i].data=static_cast<LogRecord*>(allocate_(requested[i],sizeof(LogRecord)));
        if(!next[i].data) {
          for(auto& ring:next)release_(ring.data);
          return false;
        }
      }
      const Ring& prior=rings_[i];
      next[i].overwritten=prior.overwritten;
      const size_t keep=std::min(prior.count,next[i].capacity);
      next[i].overwritten+=prior.count-keep;
      for(size_t j=0;j<keep;++j){
        const size_t index=(prior.head+prior.capacity-keep+j)%prior.capacity;
        next[i].data[j]=prior.data[index];
      }
      next[i].count=keep;
      next[i].head=keep==next[i].capacity?0:keep;
    }
    for(size_t i=0;i<rings_.size();++i){
      release_(rings_[i].data);
      rings_[i]=next[i];
    }
    limits_=limits;
    ready_=true;
    return true;
  }

  void push(LogStream stream,LogCode code,uint32_t uptime_ms,
            uint8_t radio=kAnyRadio,LogSeverity level=LogSeverity::info,
            uint16_t count=0,int32_t value=0,uint32_t correlation_id=0,
            uint8_t flags=0,uint16_t profile=0,uint8_t transport=255,
            uint8_t tx_stage=0) {
    const auto stream_id=static_cast<unsigned>(stream);
    if(stream_id>3)return;
    if((stream==LogStream::signal||stream==LogStream::noise) && radio>=3)return;
    std::lock_guard<std::mutex> lock(mutex_);
    const size_t index=stream==LogStream::signal?radio:
                       stream==LogStream::noise?3:stream==LogStream::system?4:5;
    Ring& ring=rings_[index];
    if(!ready_)return;
    const uint64_t sequence=next_sequence_++;
    if(!ring.capacity) {++ring.overwritten;return;}
    ring.data[ring.head]={sequence,uptime_ms,correlation_id,value,
      static_cast<uint16_t>(code),count,static_cast<uint8_t>(stream),radio,
      static_cast<uint8_t>(level),flags,profile,transport,tx_stage};
    ring.head=(ring.head+1)%ring.capacity;
    if(ring.count<ring.capacity)++ring.count;
    else ++ring.overwritten;
  }

  // Returns latest matching records in chronological order, without heap
  // allocation. Caller-owned output avoids exposing a live buffer to HTTP.
  size_t recent(LogRecord* out,size_t maximum,LogQuery query={}) const {
    if(!out||!maximum||query.stream>3||query.stream< -1||
       query.radio>2||query.radio< -1)return 0;
    std::lock_guard<std::mutex> lock(mutex_);
    size_t used=0;
    for(const Ring& ring:rings_) {
      for(size_t n=0;n<ring.count;++n){
        const auto& record=ring.data[(ring.head+ring.capacity-ring.count+n)%ring.capacity];
        if(query.stream>=0 && record.stream!=static_cast<uint8_t>(query.stream))continue;
        if(record.stream==static_cast<uint8_t>(LogStream::noise) &&
           !query.include_noise && query.stream!=static_cast<int>(LogStream::noise))continue;
        if(query.radio>=0 && record.radio!=static_cast<uint8_t>(query.radio))continue;
        if(used==maximum && record.sequence<=out[0].sequence)continue;
        if(used==maximum){
          for(size_t j=1;j<used;++j)out[j-1]=out[j];
          --used;
        }
        size_t j=used++;
        while(j>0 && out[j-1].sequence>record.sequence){
          out[j]=out[j-1];
          --j;
        }
        out[j]=record;
      }
    }
    return used;
  }

  LogStats stats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    LogStats s{};
    s.limits=limits_;
    s.allocated_bytes=ready_?bytes_for(limits_):0;
    s.ready=ready_;
    for(size_t i=0;i<rings_.size();++i) {
      s.used[i]=static_cast<uint32_t>(rings_[i].count);
      s.overwritten[i]=rings_[i].overwritten;
    }
    return s;
  }

 private:
  struct Ring {
    LogRecord* data=nullptr;
    size_t capacity=0;
    size_t count=0;
    size_t head=0;
    uint64_t overwritten=0;
  };
  Allocator allocate_;
  Deallocator release_;
  mutable std::mutex mutex_;
  std::array<Ring,6> rings_{};
  LogLimits limits_{};
  uint64_t next_sequence_=1;
  bool ready_=false;
};
} // namespace rflink
