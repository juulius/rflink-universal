"""Decoder manifest schema 2: stdlib-only validation and C++ capability codegen.

This module is deliberately shipped in the ESPHome component, so firmware generation
can rebuild the catalog without an external Python installation or a core edit.
"""
import json
import re
from pathlib import Path

ID = re.compile(r"^[a-z][a-z0-9_]*$")
PROTO = re.compile(r"^[A-Za-z][A-Za-z0-9_]*$")
KEY = re.compile(r"^[a-z][a-z0-9_]*$")
TX_ENCODERS = {"none", "nexa", "byron_sx", "dooya", "rc_switch", "somfy_rts_56", "itho_rft", "ramses_ii"}  # Encoder registration is a separate, audited step.
TX_STATES = {"disabled", "host_tested", "hardware_verified"}
HA_TYPES = {"sensor", "binary_sensor"}
REQUIRED = {"schema", "id", "name", "description", "bands", "modes", "device_types",
            "pairing", "tx", "maturity", "difficulty", "identity_note", "host_test", "enabled", "capabilities"}


def load_manifest(path):
    try:
        from .radio_transport_manifest import validate_radio_transport
    except ImportError:  # direct file-based host tests (no Python package context)
        import importlib.util
        spec = importlib.util.spec_from_file_location("radio_transport_manifest", Path(__file__).with_name("radio_transport_manifest.py"))
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        validate_radio_transport = module.validate_radio_transport
    m = json.loads(Path(path).read_text(encoding="utf-8"))
    pack = Path(path).parent.name
    errors = []
    if REQUIRED - set(m):
        errors.append(f"missing keys {sorted(REQUIRED - set(m))}")
    if m.get("schema") != 2 or m.get("id") != pack or not ID.fullmatch(pack):
        errors.append("schema must be 2 and id must equal a valid folder name")
    capabilities = m.get("capabilities")
    if not isinstance(capabilities, list) or not capabilities:
        errors.append("capabilities must be a nonempty list")
        capabilities = []
    for cap in capabilities:
        if not isinstance(cap, dict) or not PROTO.fullmatch(str(cap.get("protocol", ""))):
            errors.append("invalid protocol name")
            continue
        learn = cap.get("learn")
        if not isinstance(learn, dict) or learn.get("category") not in {"sensor", "control", "development"} or any(
            not isinstance(learn.get(k), str) or not learn[k].strip() or len(learn[k]) > 350
            for k in ("band", "instruction", "ha_result", "warning")
        ):
            errors.append(f"{cap['protocol']}: missing or invalid end-user learning instructions")
        if not isinstance(cap.get("rx_actions"), list) or not all(isinstance(x, str) and (KEY.fullmatch(x) or (x.endswith("_*") and KEY.fullmatch(x[:-1]+"x"))) for x in cap["rx_actions"]):
            errors.append(f"{cap['protocol']}: rx_actions must be names or suffix wildcards")
        fields = cap.get("fields", [])
        if not isinstance(fields, list):
            errors.append(f"{cap['protocol']}: fields must be a list")
            fields = []
        seen_fields = set()
        for f in fields:
            fid = str(f.get("id", "")) if isinstance(f, dict) else ""
            is_internal_pattern = fid == "check_button_*" and cap["protocol"] == "Dooya"
            if not isinstance(f, dict) or not (KEY.fullmatch(fid) or is_internal_pattern):
                errors.append(f"{cap['protocol']}: invalid field")
                continue
            if f["id"] in seen_fields:
                errors.append(f"{cap['protocol']}: duplicate field {f['id']}")
            seen_fields.add(f["id"])
            ha = f.get("ha")
            if ha is not None and (not isinstance(ha, dict) or ha.get("type") not in HA_TYPES or
                                   not isinstance(ha.get("label"), str) or not ha.get("label") or
                                   not isinstance(ha.get("device_class", ""), str) or
                                   not isinstance(ha.get("unit", ""), str)):
                errors.append(f"{cap['protocol']}: invalid HA profile for {f['id']}")
        tx = cap.get("transmit")
        if not isinstance(tx, dict):
            errors.append(f"{cap['protocol']}: missing transmit definition")
            continue
        encoder, status, actions = tx.get("encoder"), tx.get("status"), tx.get("actions")
        if not isinstance(tx.get("note", ""), str) or len(tx.get("note", "")) > 180:
            errors.append(f"{cap['protocol']}: TX note must be readable text <=180 characters")
        if encoder not in TX_ENCODERS or status not in TX_STATES or not isinstance(actions, list):
            errors.append(f"{cap['protocol']}: invalid transmit definition")
            continue
        if tx.get("default_enabled") is not False:
            errors.append(f"{cap['protocol']}: transmit default_enabled must be false")
        if (encoder == "none") != (status == "disabled" and not actions):
            errors.append(f"{cap['protocol']}: disabled TX needs encoder none and no actions")
        if encoder != "none" and (not actions or not tx.get("radio") or not isinstance(tx.get("required_fields"), list)):
            errors.append(f"{cap['protocol']}: active TX needs actions, radio and required_fields")
        # A decoder cannot grant a reviewed encoder to an unrelated protocol,
        # select an untested frequency, or silently change the encoder inputs.
        # New encoders require an explicit code/review change here first.
        if encoder == "nexa" and (cap["protocol"] != "Nexa" or
                                   tx.get("radio") != "433.92" or
                                   tx.get("status") not in ("host_tested", "hardware_verified") or
                                   set(tx.get("required_fields", [])) != {"channel", "group", "level", "tx_radio"} or
                                   {a.get("id") for a in actions if isinstance(a, dict)} != {"on", "off"}):
            errors.append(f"{cap['protocol']}: nexa encoder requires the reviewed Nexa/433.92 on/off contract")
        if encoder == "byron_sx" and (cap["protocol"] != "ByronSX" or
                                      tx.get("radio") != "433.92" or
                                      tx.get("status") not in ("host_tested", "hardware_verified") or
                                      set(tx.get("required_fields", [])) != {"tx_radio"} or
                                      {a.get("id") for a in actions if isinstance(a, dict)} != {"command_*"}):
            errors.append(f"{cap['protocol']}: byron_sx encoder requires reviewed Byron 433.92 command_* contract")
        if encoder == "dooya" and (cap["protocol"] != "Dooya" or
                                     tx.get("radio") != "433.92" or
                                     tx.get("status") not in ("host_tested", "hardware_verified") or
                                     set(tx.get("required_fields", [])) != {"channel", "check_button_*", "tx_radio"} or
                                     {a.get("id") for a in actions if isinstance(a, dict)} != {"button_*"} or
                                     not {"channel", "check_button_*", "tx_radio"}.issubset(seen_fields)):
            errors.append(f"{cap['protocol']}: dooya requires per-button check snapshots and 433.92 radio")
        if encoder == "rc_switch" and (cap["protocol"] != "RCSwitch" or
                                         tx.get("radio") != "433.92" or
                                         tx.get("status") not in ("host_tested", "hardware_verified") or
                                         set(tx.get("required_fields", [])) != {"protocol", "code", "bit_length", "tx_radio"} or
                                         {a.get("id") for a in actions if isinstance(a, dict)} != {"code"} or
                                         not {"protocol", "code", "bit_length", "tx_radio"}.issubset(seen_fields)):
            errors.append(f"{cap['protocol']}: RC-Switch requires explicit code/protocol/bit length and 433.92")
        if encoder == "somfy_rts_56" and (cap["protocol"] != "SomfyRTS56" or
                                            tx.get("radio") != "433.42" or
                                            tx.get("status") not in ("host_tested", "hardware_verified") or
                                            set(tx.get("required_fields", [])) != {"somfy_address", "somfy_rolling_code", "somfy_paired"} or
                                            {a.get("id") for a in actions if isinstance(a, dict)} != {"up", "down", "my", "prog"}):
            errors.append(f"{cap['protocol']}: Somfy RTS encoder requires reviewed 56-bit 433.42 rolling-code contract")
        if encoder == "itho_rft" and (cap["protocol"] != "IthoRFT" or
                                        tx.get("radio") != "868.30" or
                                        tx.get("status") not in ("host_tested", "hardware_verified") or
                                        set(tx.get("required_fields", [])) != {"itho_address", "itho_sequence", "itho_device_type", "itho_paired"} or
                                        {a.get("id") for a in actions if isinstance(a, dict)} != {"standby", "low", "medium", "high", "timer_10", "timer_20", "timer_30"}):
            errors.append(f"{cap['protocol']}: Itho RFT encoder requires reviewed 868.30 packet/virtual-remote contract")
        if encoder == "ramses_ii" and (cap["protocol"] != "RamsesII" or
            tx.get("radio") != "868.30" or tx.get("status") != "host_tested" or
            set(tx.get("required_fields", [])) != {"ramses_controller", "ramses_fan", "ramses_paired"} or
            {a.get("id") for a in actions if isinstance(a,dict)} !=
            {"away","auto","low","medium","high","timer_15","timer_30","timer_60"}):
            errors.append(f"{cap['protocol']}: RAMSES-II requires experimental approved 868.30 GDO serial contract")
        for act in actions:
            if not isinstance(act, dict) or not (KEY.fullmatch(str(act.get("id", ""))) or (str(act.get("id", "")).endswith("_*") and KEY.fullmatch(str(act.get("id", ""))[:-1]+"x"))) or \
                act.get("entity") != "button" or not act.get("label") or act.get("id") not in cap.get("rx_actions", []):
                errors.append(f"{cap['protocol']}: invalid/unapproved TX action {act}")
        if len({a.get("id") for a in actions if isinstance(a, dict)}) != len(actions):
            errors.append(f"{cap['protocol']}: duplicate TX action")
    try:
        validate_radio_transport(m)
    except (ValueError, KeyError, TypeError) as error:
        errors.append(str(error))
    try:
        from .radio_contract_v2 import validate_contract
    except ImportError:
        import importlib.util
        spec = importlib.util.spec_from_file_location("radio_contract_v2", Path(__file__).with_name("radio_contract_v2.py"))
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        validate_contract = module.validate_contract
    try:
        validate_contract(m)
    except (ValueError, KeyError, TypeError) as error:
        errors.append(str(error))
    if errors:
        raise ValueError(f"{pack}/manifest.json: " + "; ".join(errors))
    return m


def validate_catalog(packs):
    """packs: sequence of (folder, manifest), enabled only; no silent overrides."""
    protocols, fields = {}, {}
    for folder, m in packs:
        if m.get("schema") != 2:
            raise ValueError(f"{folder.name}: schema 2 required")
        for cap in m["capabilities"]:
            name = cap["protocol"]
            if name in protocols:
                raise ValueError(f"protocol {name} declared twice in {protocols[name][0]} and {folder.name}")
            protocols[name] = (folder.name, cap)
            for f in cap["fields"]:
                if f.get("ha") is None:
                    continue
                key = f["id"]
                if key in fields and fields[key] != f["ha"]:
                    raise ValueError(f"conflicting HA sensor schema for '{key}'")
                fields[key] = f["ha"]
    return protocols, fields


def _quoted(value):
    # JSON strings are valid C++ string literals. Use ASCII escapes for \n and quotes,
    # but keep UTF-8 human-readable labels in the generated header.
    return json.dumps(value, ensure_ascii=False)


def generate_header(packs):
    protocols, fields = validate_catalog(packs)
    rows = ["// AUTOGENERATED from decoder manifests schema 2; DO NOT EDIT.\n",
            "#pragma once\n#include <cstddef>\n#include <cstring>\n",
            "namespace esphome { namespace rflink_universal { namespace capabilities {\n",
            "struct Protocol { const char *name, *pack, *rx_actions, *fields, *encoder, *tx_status, *tx_radio, *tx_actions, *tx_note, *learn_category, *learn_band, *learn_instruction, *learn_ha_result, *learn_warning; };\n",
            "struct Field { const char *key, *display, *kind, *device_class, *unit; };\n",
            "inline bool contains(const char *tokens, const char *needle) { if (!tokens || !needle || !*needle) return false; const size_t n=std::strlen(needle); const char *p=tokens; while (*p) { const char *end=std::strchr(p,'|'); const size_t len=end ? static_cast<size_t>(end-p) : std::strlen(p); if ((len==n && std::strncmp(p,needle,n)==0) || (len>=2 && p[len-1]=='*' && n>len-1 && std::strncmp(p,needle,len-1)==0)) return true; if (!end) break; p=end+1; } return false; }\n",
            "inline const Protocol *protocols(size_t &count) { static const Protocol data[] = {\n"]
    for name, (pack, cap) in sorted(protocols.items()):
        tx = cap["transmit"]
        learn = cap["learn"]
        cells = [name, pack, "|".join(cap["rx_actions"]), "|".join(f["id"] for f in cap["fields"]),
                 tx["encoder"], tx["status"], tx.get("radio", ""), "|".join(a["id"] for a in tx["actions"]), tx.get("note", ""),
                 learn["category"], learn["band"], learn["instruction"], learn["ha_result"], learn["warning"]]
        rows.append("  {" + ", ".join(map(_quoted, cells)) + "},\n")
    rows.extend(["}; count=sizeof(data)/sizeof(data[0]); return data; }\n",
                 "inline const Protocol *find(const char *name) { size_t count=0; const auto *all=protocols(count); for(size_t i=0;i<count;i++) if(std::strcmp(all[i].name,name)==0) return &all[i]; return nullptr; }\n",
                 "inline bool tx_allowed(const char *protocol, const char *action) { const auto *p=find(protocol); return p && std::strcmp(p->encoder,\"none\")!=0 && contains(p->tx_actions,action); }\n",
                 "inline const Field *fields(size_t &count) { static const Field data[] = {\n"])
    for key, ha in sorted(fields.items()):
        cells = [key, ha["label"], ha["type"], ha.get("device_class", ""), ha.get("unit", "")]
        rows.append("  {" + ", ".join(map(_quoted,cells)) + "},\n")
    rows.extend(["}; count=sizeof(data)/sizeof(data[0]); return data; }\n",
                 "inline const Field *field(const char *key) { size_t count=0; const auto *all=fields(count); for(size_t i=0;i<count;i++) if(std::strcmp(all[i].key,key)==0) return &all[i]; return nullptr; }\n",
                 "}}} // namespace esphome::rflink_universal::capabilities\n"])
    return "".join(rows)
