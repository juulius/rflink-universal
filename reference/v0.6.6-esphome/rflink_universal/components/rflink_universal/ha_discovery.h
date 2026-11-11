#pragma once

// Pure, ESPHome-independent MQTT Discovery encoding. Only the explicit,
// semantically understood decoder fields are exported (no RF identifiers,
// rolling codes or arbitrary decoder internals).
#include <cctype>
#include <cstdio>
#include <string>
#include <vector>
#include "device_store.h"
#include "tx_policy.h"
#include "capability_catalog.h"

namespace esphome { namespace rflink_universal { namespace ha {

// Sensor/entity metadata is generated from each decoder's manifest.json.
// Unknown or private RF fields never become Home Assistant entities.
using FieldProfile = capabilities::Field;
inline const FieldProfile *profile(const std::string &key) {
  return capabilities::field(key.c_str());
}
inline bool has_events(const DeviceRecord &d) {
  const auto *p = capabilities::find(d.protocol.c_str());
  return !d.actions.empty() && !(p && std::strcmp(p->learn_category, "sensor") == 0);
}

inline std::string escape(const std::string &s) {
  std::string out;
  out.reserve(s.size() + 8);
  static constexpr char hex[] = "0123456789abcdef";
  for (unsigned char c : s) {
    if (c == '"') out += "\\\"";
    else if (c == '\\') out += "\\\\";
    else if (c == '\n') out += "\\n";
    else if (c == '\r') out += "\\r";
    else if (c == '\t') out += "\\t";
    else if (c < 0x20) {
      out += "\\u00";
      out += hex[c >> 4];
      out += hex[c & 15];
    } else out += static_cast<char>(c);
  }
  return out;
}
inline std::string quoted(const std::string &s) { return "\"" + escape(s) + "\""; }

inline std::string safe_node(std::string node) {
  for (char &c : node) if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-')) c = '_';
  return node.empty() ? "rflink" : node;
}
inline std::string prefix(const std::string &node, const DeviceRecord &d) {
  return "rflink/" + safe_node(node) + "/" + d.uid;
}
inline std::string unique(const std::string &node, const DeviceRecord &d, const std::string &key) {
  return "rflink_" + safe_node(node) + "_" + d.uid + "_" + key;
}
inline std::string discovery_topic(const std::string &node, const DeviceRecord &d, const std::string &key) {
  const auto *p = profile(key);
  const std::string kind = p ? p->kind : "event";
  return "homeassistant/" + kind + "/" + unique(node, d, key) + "/config";
}
inline std::string state_topic(const std::string &node, const DeviceRecord &d, const std::string &key) {
  return prefix(node, d) + "/" + key + "/state";
}
inline bool has_entities(const DeviceRecord &d) {
  if (has_events(d)) return true;
  for (const auto &f : d.fields) if (profile(f.key) != nullptr) return true;
  return false;
}
inline std::string device_json(const std::string &node, const DeviceRecord &d) {
  return "{\"identifiers\":[" + quoted("rflink_" + safe_node(node) + "_" + d.uid) +
         "],\"name\":" + quoted(d.name) + ",\"manufacturer\":\"RFLink Universal\",\"model\":" + quoted(d.protocol) + "}";
}
inline std::string field_discovery(const std::string &node, const DeviceRecord &d, const FieldProfile &p) {
  const std::string key = p.key;
  std::string json = "{\"name\":" + quoted(p.display) + ",\"unique_id\":" + quoted(unique(node, d, key)) +
                     ",\"state_topic\":" + quoted(state_topic(node, d, key)) +
                     ",\"availability_topic\":" + quoted(safe_node(node) + "/status") + ",\"device\":" + device_json(node, d);
  if (*p.device_class) json += ",\"device_class\":" + quoted(p.device_class);
  if (*p.unit) json += ",\"unit_of_measurement\":" + quoted(p.unit);
  if (key == "battery_low") json += ",\"payload_on\":\"true\",\"payload_off\":\"false\"";
  return json + "}";
}
inline std::string event_discovery(const std::string &node, const DeviceRecord &d) {
  if (!has_events(d)) return {};
  std::string json = "{\"name\":\"Knoppen\",\"unique_id\":" + quoted(unique(node, d, "action")) +
                     ",\"state_topic\":" + quoted(state_topic(node, d, "action")) +
                     ",\"availability_topic\":" + quoted(safe_node(node) + "/status") + ",\"device\":" + device_json(node, d) + ",\"event_types\":[";
  for (size_t i = 0; i < d.actions.size(); i++) {
    if (i) json += ',';
    json += quoted(d.actions[i]);
  }
  return json + "]}";
}
inline std::string button_topic(const std::string &node, const DeviceRecord &d, const std::string &action) {
  return "homeassistant/button/" + unique(node,d,"tx_"+action) + "/config";
}
inline std::string command_topic(const std::string &node, const DeviceRecord &d, const std::string &action) {
  return prefix(node,d) + "/" + action + "/set";
}
inline std::string button_discovery(const std::string &node, const DeviceRecord &d,
                                   const std::string &action, const std::string &session) {
  return "{\"name\":" + quoted(action == "on" ? "Aan" : action == "off" ? "Uit" : action) +
      ",\"unique_id\":" + quoted(unique(node,d,"tx_"+action)) +
      ",\"command_topic\":" + quoted(command_topic(node,d,action)) +
      ",\"payload_press\":" + quoted(session) +
      ",\"availability_topic\":" + quoted(safe_node(node)+"/status") +
      ",\"device\":" + device_json(node,d) + "}";
}
inline std::string signature(const DeviceRecord &d) {
  std::string sig = d.name + "\x1f" + d.protocol + (d.transmit_enabled ? "\x1ftx" : "\x1fno_tx") +
                    (d.local_transmit_enabled ? "\x1fuser_tx" : "\x1fno_user_tx");
  if (has_events(d)) for (const auto &a : d.actions) sig += "\x1e" + a;
  for (const auto &f : d.fields) if (profile(f.key)) sig += "\x1d" + f.key;
  return sig;
}
}}} // namespaces
