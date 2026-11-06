#pragma once
#include <cstdint>
#include "esp_err.h"
using esp_event_base_t=const char*;
inline constexpr esp_event_base_t WIFI_EVENT="WIFI_EVENT", IP_EVENT="IP_EVENT";
constexpr int32_t WIFI_EVENT_STA_START=1,WIFI_EVENT_STA_DISCONNECTED=2,IP_EVENT_STA_GOT_IP=3,ESP_EVENT_ANY_ID=-1;
extern "C" esp_err_t esp_event_loop_create_default();
extern "C" esp_err_t esp_event_handler_register(esp_event_base_t,int32_t,void(*)(void*,esp_event_base_t,int32_t,void*),void*);
