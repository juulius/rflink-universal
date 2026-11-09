#pragma once
// Native ESP-IDF crash report interface. The public JSON is a deliberately
// minimal summary, never raw task stacks or saved credentials.
#include <string>
#include "rflink_boot_phases.hpp"
namespace rflink {
void crash_report_record_boot();
// Write only a few well-defined startup checkpoints per boot, never RF events.
// This is a last-reached-stage marker, not a persistent serial log.
void crash_report_mark_boot_phase(BootPhase phase, bool startup_complete = false);
std::string crash_report_json();
}
