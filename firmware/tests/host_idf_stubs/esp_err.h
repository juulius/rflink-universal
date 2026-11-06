#pragma once
using esp_err_t=int;
constexpr esp_err_t ESP_FAIL=-1;
constexpr esp_err_t ESP_OK=0,ESP_ERR_INVALID_ARG=1,ESP_ERR_NO_MEM=2,ESP_ERR_INVALID_STATE=3,ESP_ERR_TIMEOUT=4,ESP_ERR_NOT_FOUND=5,ESP_ERR_INVALID_RESPONSE=6;
extern "C" const char* esp_err_to_name(esp_err_t);
#define ESP_ERROR_CHECK(x) (void)(x)
