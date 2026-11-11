#pragma once
#include <cstddef>
#include "esp_err.h"
using nvs_handle_t=unsigned;
constexpr int NVS_READONLY=1,NVS_READWRITE=2;
constexpr esp_err_t ESP_ERR_NVS_NOT_FOUND=0x1102;
extern "C" esp_err_t nvs_open(const char*,int,nvs_handle_t*);
extern "C" esp_err_t nvs_get_str(nvs_handle_t,const char*,char*,size_t*);
extern "C" esp_err_t nvs_set_str(nvs_handle_t,const char*,const char*);
extern "C" esp_err_t nvs_commit(nvs_handle_t);
extern "C" void nvs_close(nvs_handle_t);
extern "C" esp_err_t nvs_get_blob(nvs_handle_t,const char*,void*,size_t*);
extern "C" esp_err_t nvs_set_blob(nvs_handle_t,const char*,const void*,size_t);
