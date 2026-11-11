#pragma once
// RFLink internal decision trace, NOT the RX/RAW/noise log.
// RAM-only, fixed-size, non-sensitive events with a thread-safe snapshot.
#include <array>
#include <cstdint>
#include <cstring>
#include <vector>
#if defined(USE_ESP32) && defined(ESP_PLATFORM)
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#endif
namespace esphome { namespace rflink_universal {
struct SystemTraceEvent {
  uint32_t sequence{0};
  uint32_t uptime_ms{0};
  char level[10]{};
  char subsystem[20]{};
  char uid[44]{};
  char message[150]{};
};
class SystemTrace {
 public:
  static constexpr size_t CAPACITY = 256;
  static constexpr size_t DEFAULT_LIMIT = 128;
  SystemTrace() {
#if defined(USE_ESP32) && defined(ESP_PLATFORM)
    mutex_=xSemaphoreCreateMutex();
#endif
  }
  ~SystemTrace() {
#if defined(USE_ESP32) && defined(ESP_PLATFORM)
    if(mutex_) vSemaphoreDelete(mutex_);
#endif
  }
  SystemTrace(const SystemTrace&)=delete;
  SystemTrace& operator=(const SystemTrace&)=delete;
  void push(uint32_t ms,const char *level,const char *subsystem,const char *uid,const char *message) {
    if(!lock_(0)) { ++dropped_;return; }
    if(limit_ == 0) { unlock_(); return; }
    // Repeated status updates should not evict the important user events.
    if(count_ && last_ms_ <= ms && ms-last_ms_ < 1250u) {
      const auto &last=ring_[(next_+CAPACITY-1)%CAPACITY];
      if(same_(last.level,level) && same_(last.subsystem,subsystem) &&
         same_(last.uid,uid) && same_(last.message,message)) {unlock_();return;}
    }
    auto &e=ring_[next_];
    e.sequence=++sequence_;
    e.uptime_ms=ms;
    copy_(e.level,level);copy_(e.subsystem,subsystem);
    copy_(e.uid,uid);copy_(e.message,message);
    next_=(next_+1)%CAPACITY;
    if(count_<limit_) ++count_;
    last_ms_=ms;
    unlock_();
  }
  std::vector<SystemTraceEvent> snapshot(uint32_t after=0,size_t max=CAPACITY) const {
    std::vector<SystemTraceEvent> result;
    if(!lock_(20))return result;
    const auto n= max<limit_?max:limit_;
    result.reserve(n);
    const size_t start=(next_+CAPACITY-count_)%CAPACITY;
    for(size_t i=0;i<count_;++i) {
      const auto &e=ring_[(start+i)%CAPACITY];
      if(e.sequence>after && result.size()<n)result.push_back(e);
    }
    unlock_();return result;
  }
  // Runtime limit, bounded by the statically allocated ring. Reducing limits
  // immediately hides older records; storage never grows with HTTP requests.
  bool set_limit(size_t new_limit) {
    if(new_limit > CAPACITY) return false;
    if(!lock_(20)) return false;
    if(new_limit < count_) count_ = new_limit;
    limit_ = new_limit;
    unlock_(); return true;
  }
  size_t limit() const { if(!lock_(20)) return 0; const auto n=limit_; unlock_(); return n; }
  uint32_t dropped() const {return dropped_;}
 private:
  template<size_t N> static void copy_(char (&dest)[N],const char *src) {
    if(!src) src="";
    std::strncpy(dest,src,N-1);dest[N-1]=0;
  }
  static bool same_(const char *left,const char *right) {
    return std::strcmp(left,right?right:"")==0;
  }
  bool lock_(unsigned timeout_ms) const {
#if defined(USE_ESP32) && defined(ESP_PLATFORM)
    return mutex_ && xSemaphoreTake(mutex_,pdMS_TO_TICKS(timeout_ms))==pdTRUE;
#else
    (void) timeout_ms;return true;
#endif
  }
  void unlock_() const {
#if defined(USE_ESP32) && defined(ESP_PLATFORM)
    xSemaphoreGive(mutex_);
#endif
  }
  mutable std::array<SystemTraceEvent,CAPACITY> ring_{};
  mutable size_t next_{0}, count_{0};
  size_t limit_{DEFAULT_LIMIT};
  mutable uint32_t last_ms_{0},sequence_{0};
  // Metric is advisory; rare contention can lose diagnostic events but not RX/TX work.
  uint32_t dropped_{0};
#if defined(USE_ESP32) && defined(ESP_PLATFORM)
  SemaphoreHandle_t mutex_{nullptr};
#endif
};
}} // namespace
