#include "rflink_crash_report.hpp"
#include "rflink_boot_history.hpp"
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>
#include "esp_core_dump.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs.h"

namespace rflink {
namespace {
constexpr char kTag[] = "rflink.crash";
constexpr char kNamespace[] = "rf_diagnostics";
constexpr char kHistoryKey[] = "boot_history";
struct CrashSnapshot {
  BootHistory history{};
  esp_reset_reason_t reset_reason = ESP_RST_UNKNOWN;
  bool history_saved = false;
  bool dump_available = false;
  bool dump_summary_available = false;
  uint32_t dump_pc = 0;
  char dump_task[16] = {};
};
CrashSnapshot snapshot{};

const char* reset_name(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_POWERON: return "Power on";
    case ESP_RST_SW: return "Software restart";
    case ESP_RST_PANIC: return "Software crash";
    case ESP_RST_INT_WDT: return "Interrupt watchdog";
    case ESP_RST_TASK_WDT: return "Task watchdog";
    case ESP_RST_WDT: return "Other watchdog";
    case ESP_RST_DEEPSLEEP: return "Deep sleep wake";
    case ESP_RST_BROWNOUT: return "Low supply voltage";
    default: return "Unknown or other reset";
  }
}

bool crash_related(esp_reset_reason_t reason) {
  return reason == ESP_RST_PANIC || reason == ESP_RST_INT_WDT ||
         reason == ESP_RST_TASK_WDT || reason == ESP_RST_WDT;
}

void clean_task_name(char (&out)[16], const char* name) {
  if (!name) return;
  for (unsigned i = 0; i < sizeof(out) - 1 && name[i]; ++i) {
    const unsigned char c = static_cast<unsigned char>(name[i]);
    out[i] = (std::isalnum(c) || c == '_' || c == '-' || c == '.') ?
      static_cast<char>(c) : '_';
  }
}

void append_record(std::string& out, const BootEntry& entry) {
  char line[200];
  std::snprintf(line, sizeof(line),
    "{\"boot\":%lu,\"reason\":\"%s\",\"reset_code\":%lu,\"has_core_dump\":%s,\"pc\":\"0x%08lx\",\"task\":\"%s\"}",
    static_cast<unsigned long>(entry.sequence),
    reset_name(static_cast<esp_reset_reason_t>(entry.reset_reason)),
    static_cast<unsigned long>(entry.reset_reason),
    entry.core_dump ? "true" : "false",
    static_cast<unsigned long>(entry.crash_pc), entry.task);
  out += line;
}
} // namespace

void crash_report_record_boot() {
  snapshot = CrashSnapshot{};
  snapshot.reset_reason = esp_reset_reason();
  snapshot.dump_available = esp_core_dump_image_check() == ESP_OK;
  if (snapshot.dump_available) {
    esp_core_dump_summary_t summary{};
    if (esp_core_dump_get_summary(&summary) == ESP_OK) {
      snapshot.dump_summary_available = true;
      snapshot.dump_pc = summary.exc_pc;
      clean_task_name(snapshot.dump_task, summary.exc_task);
    }
  }
  nvs_handle_t handle{};
  if (nvs_open(kNamespace, NVS_READWRITE, &handle) != ESP_OK) {
    ESP_LOGW(kTag, "Unable to open persistent boot history; current reset remains available");
    return;
  }
  BootHistory history{};
  size_t length = sizeof(history);
  if (nvs_get_blob(handle, kHistoryKey, &history, &length) != ESP_OK ||
      length != sizeof(history) || !boot_history_valid(history)) {
    history = BootHistory{};
  }
  BootEntry entry{};
  entry.reset_reason = static_cast<uint32_t>(snapshot.reset_reason);
  // A core dump can remain in flash after subsequent normal restarts.
  // Do not incorrectly attach that older dump to a fresh software restart.
  if (crash_related(snapshot.reset_reason) && snapshot.dump_summary_available) {
    entry.core_dump = 1;
    entry.crash_pc = snapshot.dump_pc;
    std::memcpy(entry.task, snapshot.dump_task, sizeof(entry.task));
  }
  boot_history_append(history, entry);
  snapshot.history = history;
  const esp_err_t saved = nvs_set_blob(handle, kHistoryKey, &history, sizeof(history));
  snapshot.history_saved = saved == ESP_OK && nvs_commit(handle) == ESP_OK;
  nvs_close(handle);
  ESP_LOGI(kTag, "Last reset: %s; saved boot history=%s; coredump=%s",
           reset_name(snapshot.reset_reason),
           snapshot.history_saved ? "yes" : "no",
           snapshot.dump_available ? "yes" : "no");
  if (snapshot.dump_summary_available) {
    ESP_LOGW(kTag, "Saved core dump: task=%s PC=0x%08lx",
             snapshot.dump_task, static_cast<unsigned long>(snapshot.dump_pc));
  }
}

std::string crash_report_json() {
  // Bounded output only: never expose the raw core dump's task stack,
  // arbitrary panic strings, NVS secrets, or paired device identifiers.
  std::string out;
  out.reserve(1400);
  out += "{\"last_reset\":\"";
  out += reset_name(snapshot.reset_reason);
  out += "\",\"history_saved\":";
  out += snapshot.history_saved ? "true" : "false";
  out += ",\"core_dump\":{\"available\":";
  out += snapshot.dump_available ? "true" : "false";
  out += ",\"summary_available\":";
  out += snapshot.dump_summary_available ? "true" : "false";
  char details[120];
  std::snprintf(details, sizeof(details), ",\"pc\":\"0x%08lx\",\"task\":\"%s\"",
                static_cast<unsigned long>(snapshot.dump_pc), snapshot.dump_task);
  out += details;
  out += "},\"history\":[";
  for (unsigned i = 0; i < snapshot.history.count; ++i) {
    if (i) out += ',';
    append_record(out, boot_history_recent(snapshot.history, i));
  }
  out += "]}";
  return out;
}
} // namespace rflink
