#pragma once
#include "esp_err.h"
#include <cstddef>
using uart_port_t=int;
constexpr uart_port_t UART_NUM_1=1;
constexpr int UART_DATA_8_BITS=8,UART_PARITY_DISABLE=0,UART_STOP_BITS_1=1,UART_HW_FLOWCTRL_DISABLE=0,UART_SCLK_DEFAULT=0,UART_PIN_NO_CHANGE=-1;
struct uart_config_t {int baud_rate;int data_bits;int parity;int stop_bits;int flow_ctrl;int source_clk;};
extern "C" bool uart_is_driver_installed(uart_port_t);
extern "C" esp_err_t uart_driver_install(uart_port_t,int,int,int,void*,int);
extern "C" esp_err_t uart_param_config(uart_port_t,const uart_config_t*);
extern "C" esp_err_t uart_set_pin(uart_port_t,int,int,int,int);
extern "C" int uart_write_bytes(uart_port_t,const void*,size_t);
extern "C" esp_err_t uart_wait_tx_done(uart_port_t,unsigned);
extern "C" esp_err_t uart_driver_delete(uart_port_t);
