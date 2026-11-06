#pragma once
#include <cstddef>
#include "freertos/FreeRTOS.h"
using QueueHandle_t=void*;
using BaseType_t=int;
extern "C" QueueHandle_t xQueueCreate(unsigned,unsigned);
extern "C" int xQueueReceive(QueueHandle_t,void*,unsigned);
extern "C" int xQueueSendFromISR(QueueHandle_t,const void*,BaseType_t*);
extern "C" void vQueueDelete(QueueHandle_t);
