#pragma once
#include <cstdint>
using TickType_t=unsigned;
// Match ESP-IDF's default scheduler rate to catch zero-tick regressions.
#define configTICK_RATE_HZ 100
#define pdMS_TO_TICKS(n) ((n)*configTICK_RATE_HZ/1000)
#define portMAX_DELAY 0xffffffffu
#define pdTRUE 1
#define pdFALSE 0

// Host-only stand-ins for FreeRTOS SMP critical sections.
using portMUX_TYPE=int;
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(mux) do{(void)(mux);}while(0)
#define portEXIT_CRITICAL(mux) do{(void)(mux);}while(0)
