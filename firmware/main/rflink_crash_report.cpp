#include "rflink_crash_report.hpp"
#include "rflink_boot_history.hpp"
#include "rflink_boot_phases.hpp"
#include "rflink_crash_identity.hpp"
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>
#include <mutex>
#include "esp_core_dump.h"
#include "esp_app_desc.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs.h"

namespace rflink {
namespace {
constexpr char kTag[] = "rflink.crash";
constexpr char kNamespace[] = "rf_diagnostics";
constexpr char kHistoryKey[] = "boot_history";
constexpr char kPhaseKey[] = "boot_phases";
struct CrashSnapshot {
  BootHistory history{};
  BootPhaseHistory phase_history{};
  uint32_t boot_sequence = 0;
  bool phases_saved = false;
  esp_reset_reason_t reset_reason = ESP_RST_UNKNOWN;
  bool history_saved = false;
  bool dump_available = false;
  bool dump_summary_available = false;
  esp_err_t dump_integrity_error = ESP_ERR_NOT_FOUND;
  uint32_t dump_pc = 0;
  char dump_task[16] = {};
  char running_version[32] = {};
  char running_elf[65] = {};
  char crashed_elf[65] = {};
};
CrashSnapshot snapshot{};
std::mutex snapshot_mutex;

void clean_version(char (&dest)[32],const char* raw,size_t available){
  if(!raw)return;
  for(size_t i=0;i<sizeof(dest)-1 && i<available && raw[i];++i){
    const unsigned char c=static_cast<unsigned char>(raw[i]);
    dest[i]=(std::isalnum(c)||c=='_'||c=='-'||c=='.'||c=='+')?
      static_cast<char>(c):'_';
  }
}
const char* dump_health(esp_err_t code) {
  if(code==ESP_OK)return "valid";
  if(code==ESP_ERR_NOT_FOUND)return "not_found";
  // This covers CRC/size failures as well as flash read errors.
  return "invalid_or_unreadable";
}
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
  const esp_app_desc_t* app=esp_app_get_description();
  if(app)clean_version(snapshot.running_version,app->version,sizeof(app->version));
  char elf[65]={};
  esp_app_get_elf_sha256(elf,sizeof(elf));
  copy_elf_sha(reinterpret_cast<const uint8_t*>(elf),sizeof(elf),snapshot.running_elf);
  snapshot.dump_integrity_error = esp_core_dump_image_check();
  snapshot.dump_available = snapshot.dump_integrity_error == ESP_OK;
  if (snapshot.dump_available) {
    esp_core_dump_summary_t summary{};
    if (esp_core_dump_get_summary(&summary) == ESP_OK) {
      snapshot.dump_summary_available = true;
      snapshot.dump_pc = summary.exc_pc;
      clean_task_name(snapshot.dump_task, summary.exc_task);
      copy_elf_sha(summary.app_elf_sha256,sizeof(summary.app_elf_sha256),
                   snapshot.crashed_elf);
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
  snapshot.boot_sequence = history.next_sequence - 1;
  const esp_err_t saved = nvs_set_blob(handle, kHistoryKey, &history, sizeof(history));
  snapshot.history_saved = saved == ESP_OK && nvs_commit(handle) == ESP_OK;

  // Use a separate NVS key so previous v0.7 boot-history structures remain
  // byte-compatible. Snapshot before overwriting the newest phase on boot.
  BootPhaseHistory phase_history{};
  size_t phase_length = sizeof(phase_history);
  if (nvs_get_blob(handle, kPhaseKey, &phase_history, &phase_length) != ESP_OK ||
      phase_length != sizeof(phase_history) || !boot_phase_history_valid(phase_history)) {
    phase_history = BootPhaseHistory{};
  }
  boot_phase_begin(phase_history, snapshot.boot_sequence);
  snapshot.phase_history = phase_history;
  snapshot.phases_saved =
      nvs_set_blob(handle, kPhaseKey, &phase_history, sizeof(phase_history)) == ESP_OK &&
      nvs_commit(handle) == ESP_OK;
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

void crash_report_mark_boot_phase(BootPhase phase, bool startup_complete) {
  // These stage writes occur only during startup; never per received packet,
  // HTTP request, Wi-Fi retry or radio recovery operation.
  std::lock_guard<std::mutex> guard(snapshot_mutex);
  if (!boot_phase_set(snapshot.phase_history, snapshot.boot_sequence,
                      phase, startup_complete)) return;
  nvs_handle_t handle{};
  if (nvs_open(kNamespace, NVS_READWRITE, &handle) != ESP_OK) {
    snapshot.phases_saved = false;
    ESP_LOGW(kTag, "Boot phase %s not persisted: NVS unavailable", boot_phase_name(phase));
    return;
  }
  const esp_err_t e=nvs_set_blob(handle, kPhaseKey, &snapshot.phase_history,
                                  sizeof(snapshot.phase_history));
  const esp_err_t committed=e==ESP_OK?nvs_commit(handle):e;
  nvs_close(handle);
  snapshot.phases_saved = committed == ESP_OK;
  if (!snapshot.phases_saved)
    ESP_LOGW(kTag, "Boot phase %s not persisted: %s",
             boot_phase_name(phase), esp_err_to_name(committed));
  else
    ESP_LOGI(kTag, "Boot checkpoint: %s%s", boot_phase_name(phase),
             startup_complete ? " (app startup completed)" : "");
}

std::string crash_report_json() {
  // No concurrent reads while OTA validation reports the final boot stage.
  std::lock_guard<std::mutex> guard(snapshot_mutex);
  // Bounded output only: never expose the raw core dump's task stack,
  // arbitrary panic strings, NVS secrets, or paired device identifiers.
  std::string out;
  out.reserve(3300);
  out += "{\"last_reset\":\"";
  out += reset_name(snapshot.reset_reason);
  out += "\",\"history_saved\":";
  out += snapshot.history_saved ? "true" : "false";
  out += ",\"running_firmware\":{\"version\":\"";
  out += snapshot.running_version;
  out += "\",\"elf_sha256\":\"";
  out += snapshot.running_elf;
  out += "\"},\"core_dump\":{\"available\":";
  out += snapshot.dump_available ? "true" : "false";
  out += ",\"summary_available\":";
  out += snapshot.dump_summary_available ? "true" : "false";
  out += ",\"integrity\":\"";
  out += dump_health(snapshot.dump_integrity_error);
  out += "\",\"elf_sha256\":\"";
  out += snapshot.crashed_elf;
  out += "\",\"build_comparison\":\"";
  out += elf_match_label(compare_elf_sha(snapshot.running_elf,snapshot.crashed_elf));
  out += "\"";
  char details[120];
  std::snprintf(details, sizeof(details), ",\"pc\":\"0x%08lx\",\"task\":\"%s\"",
                static_cast<unsigned long>(snapshot.dump_pc), snapshot.dump_task);
  out += details;
  out += "},\"history\":[";
  for (unsigned i = 0; i < snapshot.history.count; ++i) {
    if (i) out += ',';
    append_record(out, boot_history_recent(snapshot.history, i));
  }
  out += "],\\"phase_history_saved\\":";
  out += snapshot.phases_saved ? "true" : "false";
  out += ",\\"boot_phases\\":[";
  bool first_phase=true;
  for (unsigned i=0; i<snapshot.history.count; ++i) {
    const auto reset=boot_history_recent(snapshot.history,i);
    const auto* phase=boot_phase_find(snapshot.phase_history,reset.sequence);
    if(!phase)continue; // Older boots (before upgrade) have no checkpoint data.
    if(!first_phase)out += ',';
    first_phase=false;
    char line[180];
    std::snprintf(line,sizeof(line),
      "{\\"boot\\":%lu,\\"stage\\":\\"%s\\",\\"startup_complete\\":%s}",
      static_cast<unsigned long>(phase->sequence),
      boot_phase_name(static_cast<BootPhase>(phase->stage)),
      phase->startup_complete ? "true" : "false");
    out += line;
  }
  out += "]}";
  return out;
}
} // namespace rflink
