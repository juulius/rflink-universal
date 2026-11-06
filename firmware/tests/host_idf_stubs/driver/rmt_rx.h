#pragma once
#include <cstddef>
#include <cstdint>
#include "esp_err.h"
#include "driver/rmt_tx.h"
#ifndef IRAM_ATTR
#define IRAM_ATTR
#endif
struct rmt_rx_done_event_data_t {size_t num_symbols;};
struct rmt_rx_event_callbacks_t {
  bool (*on_recv_done)(rmt_channel_handle_t,const rmt_rx_done_event_data_t*,void*)=nullptr;
};
struct rmt_rx_channel_config_t {
  int gpio_num=0;
  rmt_clock_source_t clk_src=0;
  uint32_t resolution_hz=0;
  size_t mem_block_symbols=0;
  struct {unsigned with_dma:1;} flags{};
};
struct rmt_receive_config_t {uint32_t signal_range_min_ns=0;uint32_t signal_range_max_ns=0;};
extern "C" esp_err_t rmt_receive(rmt_channel_handle_t,void*,size_t,const rmt_receive_config_t*);
extern "C" esp_err_t rmt_new_rx_channel(const rmt_rx_channel_config_t*,rmt_channel_handle_t*);
extern "C" esp_err_t rmt_rx_register_event_callbacks(rmt_channel_handle_t,const rmt_rx_event_callbacks_t*,void*);
