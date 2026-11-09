#pragma once
#include <cstdint>
#include "esp_err.h"
// Minimal ESP-IDF v5.5 clock-tree API for RMT host syntax/runtime tests.
using soc_module_clk_t = int;
using esp_clk_tree_src_freq_precision_t = int;
constexpr esp_clk_tree_src_freq_precision_t ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED = 0;
extern "C" esp_err_t esp_clk_tree_src_get_freq_hz(
    soc_module_clk_t source, esp_clk_tree_src_freq_precision_t precision,
    uint32_t* source_hz);
