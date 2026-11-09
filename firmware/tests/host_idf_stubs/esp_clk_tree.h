#pragma once
#include <cstdint>
#include "esp_err.h"
using soc_module_clk_t=int;
using esp_clk_tree_src_freq_precision_t=int;
constexpr int ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED=0;
extern "C" esp_err_t esp_clk_tree_src_get_freq_hz(
  soc_module_clk_t,esp_clk_tree_src_freq_precision_t,uint32_t*);
