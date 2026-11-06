#pragma once
using SemaphoreHandle_t=void*;
extern "C" void* xSemaphoreCreateMutex();
extern "C" int xSemaphoreTake(void*,unsigned);
extern "C" int xSemaphoreGive(void*);
extern "C" void vSemaphoreDelete(void*);
