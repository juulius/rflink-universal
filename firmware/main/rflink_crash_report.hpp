#pragma once
// Native ESP-IDF crash report interface. The public JSON is a deliberately
// minimal summary, never raw task stacks or saved credentials.
#include <string>
namespace rflink {
void crash_report_record_boot();
std::string crash_report_json();
}
