#pragma once
#include "esp_err.h"
using gpio_num_t=int;
constexpr int GPIO_MODE_OUTPUT=1;
extern "C" esp_err_t gpio_set_direction(gpio_num_t,int);
extern "C" esp_err_t gpio_set_level(gpio_num_t,int);
extern "C" esp_err_t gpio_reset_pin(gpio_num_t);
extern "C" int gpio_get_level(gpio_num_t);
