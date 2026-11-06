#pragma once
#include <cstddef>
#include "esp_err.h"
using spi_device_handle_t=void*;
constexpr int SPI2_HOST=2;
constexpr int SPI_DMA_DISABLED=0;
struct spi_bus_config_t {int mosi_io_num=0,miso_io_num=0,sclk_io_num=0,quadwp_io_num=0,quadhd_io_num=0,max_transfer_sz=0;};
struct spi_device_interface_config_t {int clock_speed_hz=0,mode=0,spics_io_num=0,queue_size=0;};
struct spi_transaction_t {size_t length=0;const void* tx_buffer=nullptr;void* rx_buffer=nullptr;};
extern "C" esp_err_t spi_bus_initialize(int,const spi_bus_config_t*,int);
extern "C" esp_err_t spi_bus_add_device(int,const spi_device_interface_config_t*,spi_device_handle_t*);
extern "C" esp_err_t spi_bus_remove_device(spi_device_handle_t);
extern "C" esp_err_t spi_bus_free(int);
extern "C" esp_err_t spi_device_polling_transmit(spi_device_handle_t,spi_transaction_t*);
