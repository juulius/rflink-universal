#pragma once
#include "esp_err.h"
struct esp_netif_t {};
extern "C" esp_err_t esp_netif_init();
extern "C" esp_netif_t* esp_netif_create_default_wifi_sta();
extern "C" esp_netif_t* esp_netif_create_default_wifi_ap();
#include <cstdint>
struct esp_ip4_addr_t {uint32_t addr;};
struct esp_netif_ip_info_t {esp_ip4_addr_t ip,netmask,gw;};
struct ip_event_got_ip_t {esp_netif_t* esp_netif; esp_netif_ip_info_t ip_info; bool ip_changed;};
#define IPSTR "%u.%u.%u.%u"
#define IP2STR(ip) ((unsigned)((ip)->addr&255)),((unsigned)(((ip)->addr>>8)&255)),((unsigned)(((ip)->addr>>16)&255)),((unsigned)(((ip)->addr>>24)&255))
extern "C" esp_netif_t* esp_netif_get_handle_from_ifkey(const char*);
extern "C" esp_err_t esp_netif_get_ip_info(esp_netif_t*,esp_netif_ip_info_t*);
