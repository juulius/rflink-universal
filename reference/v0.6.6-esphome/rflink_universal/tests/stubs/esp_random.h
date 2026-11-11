#pragma once
#include <cstdint>
inline uint32_t esp_random(){ static uint32_t v=0xAB12345u; return v+=0x9E3779B9u; }
