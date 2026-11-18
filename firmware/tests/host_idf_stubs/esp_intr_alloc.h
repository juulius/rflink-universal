#pragma once
// Host syntax audit only, no ISR allocation or actual flash-cache safety.
#define ESP_INTR_FLAG_IRAM (1 << 10)
