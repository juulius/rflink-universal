#pragma once
// Host syntax audit only. The real ESP-IDF esp_netif_sntp.h provides SNTP.
#include <sys/time.h>
#include "esp_err.h"
struct esp_sntp_config_t {
  bool start=true;
  void (*sync_cb)(struct timeval*)=nullptr;
};
#define ESP_NETIF_SNTP_DEFAULT_CONFIG(server_name) esp_sntp_config_t{}
extern "C" esp_err_t esp_netif_sntp_init(const esp_sntp_config_t*);
