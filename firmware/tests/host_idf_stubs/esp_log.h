#pragma once
#define ESP_LOGE(...) do{}while(0)
#define ESP_LOGW(...) do{}while(0)
#define ESP_LOGI(...) do{}while(0)
#include <cstdarg>
using vprintf_like_t=int(*)(const char*,va_list);
extern "C" vprintf_like_t esp_log_set_vprintf(vprintf_like_t);
