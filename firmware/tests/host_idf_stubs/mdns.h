#pragma once
#include "esp_err.h"
#include <cstddef>

// Host-only signatures for the managed Espressif mDNS component. The actual
// firmware build uses the pinned espressif/mdns header, not these stubs.
struct mdns_txt_item_t {
  const char* key;
  const char* value;
};
extern "C" esp_err_t mdns_init();
extern "C" esp_err_t mdns_hostname_set(const char*);
extern "C" esp_err_t mdns_instance_name_set(const char*);
extern "C" esp_err_t mdns_service_add(const char*,const char*,const char*,unsigned short,
                                      const mdns_txt_item_t*,size_t);
