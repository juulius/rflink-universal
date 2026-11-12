#pragma once
// Trustworthy wall-clock evidence is optional. RF receive never reads NVS,
// waits on SNTP or allocates; its monotonic millisecond stamps remain primary.
#include <cstdint>
namespace rflink {
void time_service_set_boot(uint32_t sequence); // after NVS init, before Wi-Fi
void time_service_start();                      // once, after station gets an IP
void time_service_poll();                       // main worker: persist at most once per boot
bool time_synchronized();                       // NTP verified in this boot
uint32_t time_boot_id();
uint64_t time_boot_started_epoch_ms(uint32_t boot);
uint64_t time_event_epoch_ms(uint32_t uptime_ms); // 0 = unknown (never fabricated)
uint64_t time_now_epoch_ms();                     // 0 = unknown
}
