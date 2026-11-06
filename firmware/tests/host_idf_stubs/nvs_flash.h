#pragma once
#include "esp_err.h"
constexpr esp_err_t ESP_ERR_NVS_NO_FREE_PAGES=100,ESP_ERR_NVS_NEW_VERSION_FOUND=101;
extern "C" esp_err_t nvs_flash_init();
