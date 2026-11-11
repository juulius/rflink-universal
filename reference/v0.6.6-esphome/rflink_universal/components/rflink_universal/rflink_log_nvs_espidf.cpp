#include "rflink_log_nvs_espidf.h"
#ifdef ESP_PLATFORM
#include <nvs.h>
#include <nvs_flash.h>
#include <cstddef>

namespace rflink_logging {
namespace {
constexpr char kNamespace[]="rflink_log";
constexpr char kKey[]="rxlog_cfg_v1";
constexpr uint32_t kMagic=0x52464C47U; // 'RFLG'
struct Persisted {
  uint32_t magic = kMagic;
  uint16_t version = 1;
  uint16_t signals[3] = {250,250,250};
  uint16_t noise = 2000;
  uint16_t floor = 50;
  uint16_t reserved = 0;
};
static_assert(sizeof(Persisted) <= 24, "Unexpected NVS config size");

bool valid(const Limits& l) {
  if (l.noise_floor > 2000) return false;
  size_t total=l.noise;
  for(auto n:l.signals) total+=n;
  return total <= kMaxLogEvents;
}
}

bool load_limits_from_nvs(Store& store) {
  if (!store.ready()) return false;
  nvs_handle_t handle;
  if (nvs_open(kNamespace, NVS_READONLY, &handle) != ESP_OK) return false;
  Persisted data{};
  size_t length=sizeof(data);
  const esp_err_t err=nvs_get_blob(handle,kKey,&data,&length);
  nvs_close(handle);
  if (err != ESP_OK || length != sizeof(data) || data.magic != kMagic || data.version != 1) return false;
  Limits limits;
  for (size_t i=0;i<3;++i) limits.signals[i]=data.signals[i];
  limits.noise=data.noise;
  limits.noise_floor=data.floor;
  return valid(limits) && store.set_limits(limits);
}

bool commit_limits_to_nvs(Store& store, const Limits& limits) {
  if (!store.ready() || !valid(limits)) return false;
  nvs_handle_t handle;
  if (nvs_open(kNamespace, NVS_READWRITE, &handle) != ESP_OK) return false;
  Persisted data{};
  for (size_t i=0;i<3;++i) data.signals[i]=limits.signals[i];
  data.noise=limits.noise;
  data.floor=limits.noise_floor;
  esp_err_t err=nvs_set_blob(handle,kKey,&data,sizeof(data));
  if (err == ESP_OK) err=nvs_commit(handle);
  nvs_close(handle);
  if (err != ESP_OK) return false;
  return store.set_limits(limits);
}
} // namespace rflink_logging
#endif
