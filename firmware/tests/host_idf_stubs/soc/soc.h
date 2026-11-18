#pragma once
// Syntax-only GPIO register read. Never use as a simulated RF measurement.
#include <cstdint>
#define REG_READ(addr) (static_cast<uint32_t>(0))
