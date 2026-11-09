#pragma once
// Native ESP-IDF crash report interface. The public JSON is a deliberately
// minimal summary, never raw task stacks or saved credentials.
#include <string>
#include "rflink_boot_phases.hpp"
#include "rflink_ota_health_report.hpp"
namespace rflink {
void crash_report_record_boot();
// Write only a few well-defined startup checkpoints per boot, never RF events.
// This is a last-reached-stage marker, not a persistent serial log.
void crash_report_mark_boot_phase(BootPhase phase, bool startup_complete = false);
// Persist a small OTA-health result before a rollback or VALID transition.
 // NVS stays shared across both firmware partitions.
bool crash_report_record_ota_health(OtaHealthOutcome outcome, bool spi, bool web,
                                    bool worker, uint32_t cycles,
                                    unsigned active_receivers, unsigned responsive_radios);
std::string crash_report_json();
}
