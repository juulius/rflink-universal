#pragma once
// Host-only syntax surface for ESP-IDF 5.5.6 espcoredump.
// Real hardware/core dump API is built and tested separately.
#include <cstdint>
#include "esp_err.h"
struct esp_core_dump_summary_t {
  uint32_t exc_pc=0;
  char exc_task[16]={};
  uint8_t app_elf_sha256[65]={};
};
extern "C" esp_err_t esp_core_dump_image_check(void);
extern "C" esp_err_t esp_core_dump_get_summary(esp_core_dump_summary_t *);
