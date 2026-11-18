#pragma once
#include "esp_err.h"
using gpio_num_t=int;
using gpio_pull_mode_t=int;
constexpr int GPIO_MODE_INPUT=0;
constexpr int GPIO_MODE_OUTPUT=1;
constexpr int GPIO_PULLDOWN_ONLY=2;
constexpr int GPIO_PULLUP_ONLY=3;
constexpr int GPIO_FLOATING=4;
extern "C" esp_err_t gpio_set_pull_mode(gpio_num_t,int);
extern "C" esp_err_t gpio_set_direction(gpio_num_t,int);
extern "C" esp_err_t gpio_set_level(gpio_num_t,int);
extern "C" esp_err_t gpio_reset_pin(gpio_num_t);
extern "C" int gpio_get_level(gpio_num_t);

constexpr int GPIO_INTR_DISABLE=0;
constexpr int GPIO_INTR_POSEDGE=1;
constexpr int GPIO_INTR_NEGEDGE=2;
constexpr int GPIO_PULLUP_DISABLE=0;
constexpr int GPIO_PULLDOWN_DISABLE=0;
#define GPIO_IS_VALID_GPIO(pin) ((pin)>=0 && (pin)<49)
// Shape-only GPIO config for the host compiler; NOT hardware behavior.
struct gpio_config_t {
  unsigned long long pin_bit_mask=0;
  int mode=0;
  int pull_up_en=0;
  int pull_down_en=0;
  int intr_type=0;
};
extern "C" esp_err_t gpio_config(const gpio_config_t*);
extern "C" esp_err_t gpio_install_isr_service(int);
extern "C" esp_err_t gpio_set_intr_type(gpio_num_t,int);
extern "C" esp_err_t gpio_isr_handler_add(gpio_num_t,void(*)(void*),void*);
extern "C" esp_err_t gpio_isr_handler_remove(gpio_num_t);
extern "C" esp_err_t gpio_intr_enable(gpio_num_t);
extern "C" esp_err_t gpio_intr_disable(gpio_num_t);
