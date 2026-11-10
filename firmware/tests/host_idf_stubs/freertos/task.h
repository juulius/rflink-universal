#pragma once
extern "C" void vTaskDelay(unsigned);
using TaskFunction_t=void(*)(void*);
extern "C" int xTaskCreate(TaskFunction_t,const char*,unsigned,void*,unsigned,void*);
#ifndef pdPASS
#define pdPASS 1
#endif

// Minimum remaining stack at any point since this FreeRTOS task started.
extern "C" unsigned uxTaskGetStackHighWaterMark(void* task);
