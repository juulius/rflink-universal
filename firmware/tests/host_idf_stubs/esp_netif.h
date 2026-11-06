#pragma once
#include "esp_err.h"
struct esp_netif_t {};
extern "C" esp_err_t esp_netif_init();
extern "C" esp_netif_t* esp_netif_create_default_wifi_sta();
extern "C" esp_netif_t* esp_netif_create_default_wifi_ap();
#include <cstdint>
struct esp_netif_ip_info_t {struct {uint32_t addr;} ip;};
extern "C" esp_netif_t* esp_netif_get_handle_from_ifkey(const char*);
extern "C" esp_err_t esp_netif_get_ip_info(esp_netif_t*,esp_netif_ip_info_t*);
