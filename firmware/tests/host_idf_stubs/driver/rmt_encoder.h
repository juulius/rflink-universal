#pragma once
#include "driver/rmt_tx.h"
struct rmt_copy_encoder_config_t {};
extern "C" esp_err_t rmt_new_copy_encoder(const rmt_copy_encoder_config_t*,rmt_encoder_handle_t*);
extern "C" esp_err_t rmt_del_encoder(rmt_encoder_handle_t);
