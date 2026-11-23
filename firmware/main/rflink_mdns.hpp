#pragma once
#include "esp_netif.h"

// Called before STA Wi-Fi starts, so DHCP advertises a stable unique name.
void rflink_set_dhcp_hostname(esp_netif_t* station);

// Best-effort unpaired DNS-SD announcement once the HTTP API is available.
// Neither failures nor reconnections may interrupt the radio manager.
bool rflink_start_service_discovery();
