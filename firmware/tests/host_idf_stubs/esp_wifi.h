#pragma once
#include <cstdint>
#include "esp_err.h"
using wifi_mode_t=int;
using wifi_interface_t=int;
constexpr int WIFI_MODE_AP=1,WIFI_MODE_STA=2,WIFI_IF_AP=1,WIFI_IF_STA=0;
constexpr int WIFI_AUTH_WPA2_PSK=3;
struct wifi_init_config_t {int dummy;};
#define WIFI_INIT_CONFIG_DEFAULT() wifi_init_config_t{}
struct wifi_ap_config_t {uint8_t ssid[32];uint8_t password[64];uint8_t ssid_len;uint8_t authmode;uint8_t max_connection;};
struct wifi_sta_config_t {uint8_t ssid[32];uint8_t password[64];};
union wifi_config_t {wifi_ap_config_t ap;wifi_sta_config_t sta;};
extern "C" esp_err_t esp_wifi_init(const wifi_init_config_t*);
extern "C" esp_err_t esp_wifi_set_mode(wifi_mode_t);
extern "C" esp_err_t esp_wifi_set_config(wifi_interface_t,wifi_config_t*);
extern "C" esp_err_t esp_wifi_start();
extern "C" esp_err_t esp_wifi_connect();
