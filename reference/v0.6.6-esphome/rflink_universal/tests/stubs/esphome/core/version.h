#pragma once
#ifndef VERSION_CODE
#define VERSION_CODE(major, minor, patch) ((major) * 10000 + (minor) * 100 + (patch))
#endif
#ifndef ESPHOME_VERSION_CODE
#define ESPHOME_VERSION_CODE VERSION_CODE(2026, 9, 1)
#endif
