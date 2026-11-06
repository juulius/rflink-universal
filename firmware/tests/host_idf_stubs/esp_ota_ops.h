#pragma once
#include "esp_err.h"
#include "esp_partition.h"
enum esp_ota_img_states_t { ESP_OTA_IMG_UNDEFINED,ESP_OTA_IMG_NEW,ESP_OTA_IMG_PENDING_VERIFY,ESP_OTA_IMG_VALID,ESP_OTA_IMG_INVALID,ESP_OTA_IMG_ABORTED };
extern "C" const esp_partition_t* esp_ota_get_running_partition();
extern "C" esp_err_t esp_ota_get_state_partition(const esp_partition_t*,esp_ota_img_states_t*);
extern "C" esp_err_t esp_ota_mark_app_valid_cancel_rollback();
extern "C" esp_err_t esp_ota_mark_app_invalid_rollback_and_reboot();
#include <cstddef>
#include "esp_app_desc.h"
using esp_ota_handle_t=unsigned;
extern "C" const esp_partition_t* esp_ota_get_next_update_partition(const esp_partition_t*);
extern "C" esp_err_t esp_ota_begin(const esp_partition_t*,size_t,esp_ota_handle_t*);
extern "C" esp_err_t esp_ota_write(esp_ota_handle_t,const void*,size_t);
extern "C" esp_err_t esp_ota_end(esp_ota_handle_t);
extern "C" esp_err_t esp_ota_abort(esp_ota_handle_t);
extern "C" esp_err_t esp_ota_get_partition_description(const esp_partition_t*,esp_app_desc_t*);
extern "C" esp_err_t esp_ota_set_boot_partition(const esp_partition_t*);
