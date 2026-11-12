#include "rflink_time.hpp"
#include <atomic>
#include <cstdint>
#include <cstring>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_netif_sntp.h"
#include "nvs.h"

namespace rflink {
namespace {
constexpr char kTag[]="rflink.time";
constexpr uint32_t kMagic=0x5246544d; // RFTM
constexpr uint32_t kVersion=1;
constexpr uint64_t kMinEpochMs=1704067200000ULL; // 2024-01-01 UTC
constexpr uint64_t kMaxEpochMs=4102444800000ULL; // 2100-01-01 UTC
constexpr size_t kEntries=12;
struct TimeEntry { uint32_t boot=0; uint32_t reserved=0; uint64_t started_ms=0; };
struct TimeHistory {
  uint32_t magic=kMagic;
  uint32_t version=kVersion;
  TimeEntry entries[kEntries]{};
};
TimeHistory saved{};
std::atomic<uint32_t> boot_id{0};
std::atomic<int64_t> boot_epoch_ms{0}; // only set by a validated SNTP callback
std::atomic<bool> pending_save{false};
std::atomic<bool> started{false};
std::atomic<bool> saved_this_boot{false};
bool valid(const TimeHistory& h){
  return h.magic==kMagic && h.version==kVersion;
}
uint64_t boot_elapsed_ms(){
  return static_cast<uint64_t>(esp_timer_get_time()/1000);
}
void on_sync(struct timeval* now){
  if(!now || now->tv_sec<0)return;
  const uint64_t epoch=uint64_t(now->tv_sec)*1000ULL+
                       uint64_t(now->tv_usec)/1000ULL;
  const uint64_t elapsed=boot_elapsed_ms();
  if(epoch<kMinEpochMs || epoch>=kMaxEpochMs || epoch<=elapsed){
    ESP_LOGW(kTag,"Ignoring implausible SNTP timestamp");
    return;
  }
  // SNTP callback runs on the TCP/IP task: never do NVS writes here.
  boot_epoch_ms.store(static_cast<int64_t>(epoch-elapsed),std::memory_order_release);
  if(!saved_this_boot.load(std::memory_order_acquire))
    pending_save.store(true,std::memory_order_release);
  ESP_LOGI(kTag,"UTC synchronized (boot=%lu, uptime=%llu ms)",
    static_cast<unsigned long>(boot_id.load()),
    static_cast<unsigned long long>(elapsed));
}
} // namespace

void time_service_set_boot(uint32_t sequence){
  boot_id.store(sequence,std::memory_order_release);
  nvs_handle_t handle{};
  if(nvs_open("rf_diagnostics",NVS_READONLY,&handle)!=ESP_OK)return;
  size_t size=sizeof(saved);
  if(nvs_get_blob(handle,"time_history_v1",&saved,&size)!=ESP_OK ||
     size!=sizeof(saved) || !valid(saved))saved=TimeHistory{};
  nvs_close(handle);
}
void time_service_start(){
  if(started.exchange(true))return;
  esp_sntp_config_t config=ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
  config.sync_cb=&on_sync;
  const esp_err_t e=esp_netif_sntp_init(&config);
  if(e!=ESP_OK){
    started.store(false);
    ESP_LOGW(kTag,"SNTP initialization failed: %s",esp_err_to_name(e));
  }
}
bool time_synchronized(){
  return boot_epoch_ms.load(std::memory_order_acquire)>0;
}
uint32_t time_boot_id(){return boot_id.load(std::memory_order_acquire);}
uint64_t time_boot_started_epoch_ms(uint32_t boot){
  if(!boot)return 0;
  if(boot==time_boot_id()){
    const auto current=boot_epoch_ms.load(std::memory_order_acquire);
    if(current>0)return static_cast<uint64_t>(current);
  }
  for(const auto& entry:saved.entries)
    if(entry.boot==boot && entry.started_ms>=kMinEpochMs &&
       entry.started_ms<kMaxEpochMs)return entry.started_ms;
  return 0;
}
uint64_t time_event_epoch_ms(uint32_t ms){
  const int64_t anchor=boot_epoch_ms.load(std::memory_order_acquire);
  if(anchor<=0)return 0;
  // Compact log entries keep 32-bit uptime. Reconstruct the closest past
  // occurrence from the 64-bit monotonic timer for >49-day continuous boots.
  const uint64_t now=boot_elapsed_ms();
  uint64_t elapsed=(now&~uint64_t(UINT32_MAX))|ms;
  if(elapsed>now && elapsed>=uint64_t(1)<<32)elapsed-=uint64_t(1)<<32;
  if(elapsed>now)return 0;
  const uint64_t epoch=static_cast<uint64_t>(anchor)+elapsed;
  return epoch>=kMinEpochMs && epoch<kMaxEpochMs?epoch:0;
}
uint64_t time_now_epoch_ms(){
  const int64_t anchor=boot_epoch_ms.load(std::memory_order_acquire);
  if(anchor<=0)return 0;
  return static_cast<uint64_t>(anchor)+boot_elapsed_ms();
}
void time_service_poll(){
  if(!pending_save.exchange(false,std::memory_order_acq_rel) ||
     saved_this_boot.load() || !time_boot_id())return;
  const uint64_t epoch=time_boot_started_epoch_ms(time_boot_id());
  if(epoch<kMinEpochMs || epoch>=kMaxEpochMs)return;
  TimeHistory updated=saved;
  updated.entries[(time_boot_id()-1)%kEntries]={time_boot_id(),0,epoch};
  nvs_handle_t handle{};
  if(nvs_open("rf_diagnostics",NVS_READWRITE,&handle)!=ESP_OK){
    ESP_LOGW(kTag,"Cannot persist time anchor (NVS unavailable)");
    return;
  }
  const esp_err_t write=nvs_set_blob(handle,"time_history_v1",&updated,sizeof(updated));
  const esp_err_t commit=write==ESP_OK?nvs_commit(handle):write;
  nvs_close(handle);
  if(commit==ESP_OK){
    saved=updated;
    saved_this_boot.store(true);
    ESP_LOGI(kTag,"UTC boot anchor saved for boot=%lu",
      static_cast<unsigned long>(time_boot_id()));
  }else ESP_LOGW(kTag,"Could not save UTC boot anchor: %s",esp_err_to_name(commit));
}
} // namespace rflink
