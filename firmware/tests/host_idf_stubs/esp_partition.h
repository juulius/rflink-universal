#pragma once
// Host syntax-only shape of ESP-IDF esp_partition.h. No real flash access.
#include <cstddef>
#include <cstdint>
enum esp_partition_type_t {
  ESP_PARTITION_TYPE_APP=0,
  ESP_PARTITION_TYPE_DATA=1,
};
enum esp_partition_subtype_t {
  ESP_PARTITION_SUBTYPE_APP_OTA_0=16,
  ESP_PARTITION_SUBTYPE_APP_OTA_1=17,
};
struct esp_partition_t {
  int subtype=16;
  size_t size=0;
  uint32_t address=0;
  int type=ESP_PARTITION_TYPE_APP;
  char label[17]={};
};
extern "C" const esp_partition_t* esp_partition_find_first(
    esp_partition_type_t type, esp_partition_subtype_t subtype, const char* label);
