#pragma once
#include <cstdint>
inline uint32_t rflink_host_millis = 123456;
inline uint32_t millis(){ return rflink_host_millis; }

inline void delay(uint32_t) {}
