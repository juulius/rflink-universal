#pragma once
#include <cstddef>
#include <cstdint>
#include "esp_err.h"
using rmt_channel_handle_t=void*;
using rmt_encoder_handle_t=void*;
using rmt_clock_source_t=int;
constexpr int RMT_CLK_SRC_DEFAULT=0;
struct rmt_symbol_word_t { unsigned duration0:15;unsigned level0:1;unsigned duration1:15;unsigned level1:1; };
struct rmt_tx_channel_config_t {
  int gpio_num=0;
  rmt_clock_source_t clk_src=0;
  uint32_t resolution_hz=0;
  size_t mem_block_symbols=0;
  size_t trans_queue_depth=0;
  struct {unsigned with_dma:1;} flags{};
};
struct rmt_transmit_config_t {int loop_count=0;struct {unsigned eot_level:1;} flags{};};
extern "C" esp_err_t rmt_new_tx_channel(const rmt_tx_channel_config_t*,rmt_channel_handle_t*);
extern "C" esp_err_t rmt_enable(rmt_channel_handle_t);
extern "C" esp_err_t rmt_disable(rmt_channel_handle_t);
extern "C" esp_err_t rmt_del_channel(rmt_channel_handle_t);
extern "C" esp_err_t rmt_transmit(rmt_channel_handle_t,rmt_encoder_handle_t,const void*,size_t,const rmt_transmit_config_t*);
extern "C" esp_err_t rmt_tx_wait_all_done(rmt_channel_handle_t,int);
