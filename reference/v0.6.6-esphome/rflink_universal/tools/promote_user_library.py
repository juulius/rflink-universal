#!/usr/bin/env python3
"""Evaluate an exported RFLink user library and merge reviewed recipes into firmware.

Usage: python tools/promote_user_library.py backup.json --check
       python tools/promote_user_library.py backup.json --merge
       python tools/promote_user_library.py backup.json --merge --replace user_x
       python tools/promote_user_library.py backup.json --check --candidate-out review/

The resulting C++ header is a compiled-in, read-only interpreter recipe; it
never loads or rewrites the live user NVS library. Hardware verification remains
a separate step before treating TX as proven on a physical device.
"""
import argparse
import json
import re
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
COMPONENT = ROOT / "components" / "rflink_universal"
DECODERS = ROOT / "decoders"
HEADER = COMPONENT / "firmware_user_recipes.h"
MANIFEST = COMPONENT / "firmware_user_recipes.json"
ID = re.compile(r"^[a-z][a-z0-9_]{0,31}$")


def cpp(value):
    if not isinstance(value, str):
        raise ValueError("expected text")
    return json.dumps(value, ensure_ascii=True)


def fingerprint(item):
    value = 2166136261
    for key in ("id", "rx_source", "tx_source"):
        for byte in item[key].encode("utf-8"):
            value = ((value ^ byte) * 16777619) & 0xffffffff
    return value


def validate_decoder(item):
    if not isinstance(item, dict) or not ID.fullmatch(item.get("id", "")) or not item["id"].startswith("user_"):
        raise ValueError("user decoder ID must begin with user_")
    if item.get("source", "user") == "firmware":
        raise ValueError("firmware recipe is already promoted")
    if item.get("enabled") is not True or type(item.get("version")) is not int or not 1 <= item["version"] <= 65535:
        raise ValueError("activate and test a versioned decoder before promotion")
    if not isinstance(item.get("name"), str) or not 0 < len(item["name"].encode()) <= 64:
        raise ValueError("decoder needs a name of up to 64 bytes")
    if item.get("kind") not in ("raw", "packet") or type(item.get("radio")) is not int or item["radio"] not in (0, 1, 2):
        raise ValueError("invalid radio or frame kind")
    if item["kind"] == "packet" and item["radio"] != 2 or item["kind"] == "raw" and item["radio"] == 2:
        raise ValueError("frame kind does not fit the board radio")
    for key in ("rx_source", "tx_source"):
        if not isinstance(item.get(key), str) or len(item[key].encode()) > 4096:
            raise ValueError("invalid or oversized RX/TX program")
    if not item["rx_source"] or not isinstance(item.get("actions"), list) or not 0 < len(item["actions"]) <= 16:
        raise ValueError("RX program or actions missing")
    if len(set(item["actions"])) != len(item["actions"]) or not all(isinstance(a, str) and ID.fullmatch(a) for a in item["actions"]):
        raise ValueError("duplicate or invalid action")
    samples = item.get("samples")
    if not isinstance(samples, list) or not 3 <= len(samples) <= 12:
        raise ValueError("record 3–12 labelled samples")
    for sample in samples:
        if not isinstance(sample, dict) or sample.get("label") not in item["actions"] or sample.get("channel") != item["radio"] or sample.get("kind") != item["kind"]:
            raise ValueError("sample label, kind or radio mismatch")
        raw, packet = sample.get("raw"), sample.get("packet")
        if not isinstance(raw, list) or not isinstance(packet, list) or len(raw) > 512 or len(packet) > 64:
            raise ValueError("sample too large")
        if not all(type(n) is int and -1000000 <= n <= 1000000 for n in raw) or not all(type(n) is int and 0 <= n <= 255 for n in packet):
            raise ValueError("sample has invalid RF data")
        if (item["kind"] == "raw" and (not raw or packet)) or (item["kind"] == "packet" and (not packet or raw)):
            raise ValueError("sample data does not match radio kind")
    if item.get("fingerprint") is not None and item["fingerprint"] != fingerprint(item):
        raise ValueError("decoder content differs from exported fingerprint")


def builtin_protocol_actions():
    """Return schema-reviewed built-in RX actions without importing project modules."""
    result = {}
    for path in sorted(DECODERS.glob("*/manifest.json")):
        data = json.loads(path.read_text(encoding="utf-8"))
        if data.get("enabled", True) is False:
            continue
        for cap in data.get("capabilities", []):
            protocol = cap.get("protocol")
            actions = cap.get("rx_actions")
            if isinstance(protocol, str) and isinstance(actions, list):
                if protocol in result:
                    raise ValueError("built-in protocol declared twice: " + protocol)
                result[protocol] = tuple(actions)
    return result


def action_matches(patterns, action):
    return any(pattern == action or (pattern.endswith("*") and action.startswith(pattern[:-1]))
               for pattern in patterns)


def validate_profile(profile, decoders, builtins):
    if not isinstance(profile, dict) or not ID.fullmatch(profile.get("id", "")) or not isinstance(profile.get("name"), str):
        raise ValueError("invalid device profile")
    protocol = profile.get("protocol")
    if (protocol not in decoders and protocol not in builtins) or not 0 < len(profile["name"].encode()) <= 64:
        raise ValueError("profile decoder is missing")
    if type(profile.get("version", 1)) is not int or not 1 <= profile.get("version", 1) <= 65535:
        raise ValueError("invalid profile version")
    actions = profile.get("expected_actions")
    if not isinstance(actions, list) or not all(isinstance(a, str) and ID.fullmatch(a) for a in actions):
        raise ValueError("profile has unrecognized actions")
    allowed = decoders[protocol]["actions"] if protocol in decoders else builtins[protocol]
    if not all(action_matches(allowed, action) for action in actions):
        raise ValueError("profile has unrecognized actions")
    if not isinstance(profile.get("learn_help", ""), str) or len(profile.get("learn_help", "").encode()) > 255:
        raise ValueError("invalid learning help")


def parse_backup(path):
    data = json.loads(Path(path).read_text(encoding="utf-8"))
    if data.get("format") != "rflink-user-library" or data.get("schema") != 1:
        raise ValueError("not an RFLink user library export")
    source = data.get("library", {})
    if source.get("schema") != 1 or not isinstance(source.get("decoders"), list) or not isinstance(source.get("profiles"), list):
        raise ValueError("invalid library schema")
    if not all(isinstance(d, dict) for d in source["decoders"]) or not all(isinstance(p, dict) for p in source["profiles"]):
        raise ValueError("decoder/profile must be an object")
    decoders = [d for d in source["decoders"] if d.get("source", "user") == "user"]
    profiles = [p for p in source["profiles"] if p.get("source", "user") == "user"]
    if not decoders:
        raise ValueError("export contains no unpromoted user decoders")
    return decoders, profiles


def generate(decoders, profiles):
    if len(decoders) > 8 or len(profiles) > 16:
        raise ValueError("firmware recipe capacity exceeded")
    names = {}
    for item in decoders:
        validate_decoder(item)
        if item["id"] in names:
            raise ValueError("duplicate decoder ID")
        names[item["id"]] = item
    builtins = builtin_protocol_actions()
    profile_names = set()
    for profile in profiles:
        validate_profile(profile, names, builtins)
        if profile["id"] in profile_names:
            raise ValueError("duplicate profile ID")
        profile_names.add(profile["id"])
    rows = ["#pragma once", '#include "user_library.h"',
            "namespace esphome { namespace rflink_universal {",
            "// Reviewed firmware recipes. Local NVS overrides recipes with the same ID.",
            "inline bool load_firmware_user_recipes(UserLibrary &library) {",
            "  std::string error;"]
    for d in decoders:
        rows += [f"  // RF_RECIPE {d['id']} {fingerprint(d)}", "  {", "    UserDecoder item;",
                 f"    item.id = {cpp(d['id'])}; item.name = {cpp(d['name'])};",
                 f"    item.version = {d['version']};",
                 f"    item.radio = {d['radio']}; item.kind = FrameKind::{d['kind'].upper()};",
                 f"    item.rx_source = {cpp(d['rx_source'])};",
                 f"    item.tx_source = {cpp(d['tx_source'])};",
                 "    item.actions = {" + ", ".join(map(cpp, d["actions"])) + "};"]
        for sample in d["samples"]:
            rows += ["    { CapturedFrame sample;",
                     f"      sample.label = {cpp(sample['label'])}; sample.channel = {sample['channel']}; sample.kind = FrameKind::{sample['kind'].upper()};",
                     "      sample.raw = {" + ", ".join(map(str, sample["raw"])) + "};",
                     "      sample.packet = {" + ", ".join(map(str, sample["packet"])) + "};",
                     "      item.samples.push_back(std::move(sample)); }"]
        rows += ["    if (!library.upsert_decoder(item, error) || !library.enable_decoder(item.id, true, error)) return false;", "  }"]
    for p in profiles:
        rows += ["  { UserProfile profile;",
                 f"    profile.id = {cpp(p['id'])}; profile.name = {cpp(p['name'])};",
                 f"    profile.version = {p.get('version', 1)};",
                 f"    profile.protocol = {cpp(p['protocol'])}; profile.learn_help = {cpp(p.get('learn_help',''))};",
                 "    profile.expected_actions = {" + ", ".join(map(cpp, p["expected_actions"])) + "};",
                 "    if (!library.upsert_profile(profile, error)) return false;", "  }"]
    rows += ["  return true;", "}", "} }", ""]
    return "\n".join(rows)


def _cpp_class_name(identifier):
    return "".join(part[:1].upper() + part[1:] for part in identifier.split("_")) + "CandidateDecoder"


def _raw_literal(source):
    marker = "RFLINK_RECIPE"
    while ')' + marker + '"' in source:
        marker += "_X"
    return f'R"{marker}({source}){marker}"'


def write_candidate_packs(folder, decoders):
    """Generate disabled RX-only decoder packs for review/native hardening."""
    folder.mkdir(parents=True, exist_ok=True)
    written = []
    for d in decoders:
        pack_id = "candidate_" + d["id"]
        target = folder / pack_id
        target.mkdir(parents=True, exist_ok=True)
        class_name = _cpp_class_name(d["id"])
        frame_kind = "RAW" if d["kind"] == "raw" else "PACKET"
        source = f'''// AUTOGENERATED REVIEW CANDIDATE - disabled by default.
// Convert the VM recipe to protocol-native code only after tests/review.
#include "esphome/components/rflink_universal/decoder_registry.h"
#include "esphome/components/rflink_universal/user_vm.h"

namespace esphome {{ namespace rflink_universal {{
class {class_name} : public RFDecoder {{
 public:
  const char *name() const override {{ return {cpp(d['id'])}; }}
  uint8_t match(const RFFrame &frame) const override {{
    if (frame.channel != {d['radio']} || frame.kind != FrameKind::{frame_kind}) return 0;
    DecodeResult result; std::string error;
    const auto *p = program_();
    return p && user_vm::receive(*p, {cpp(d['id'])}, frame, result, error) ? 90 : 0;
  }}
  bool decode(const RFFrame &frame, DecodeResult &result) const override {{
    if (frame.channel != {d['radio']} || frame.kind != FrameKind::{frame_kind}) return false;
    std::string error; const auto *p = program_();
    if (!p || !user_vm::receive(*p, {cpp(d['id'])}, frame, result, error)) return false;
    result.protocol = {cpp(d['id'])};
    result.device_type = "remote";
    return true;
  }}
 private:
  static const user_vm::Program *program_() {{
    struct Holder {{ user_vm::Program program; bool ok{{false}}; Holder() {{
      std::string error; ok = user_vm::compile({_raw_literal(d['rx_source'])}, user_vm::Mode::RX, program, error);
    }} }};
    static const Holder holder; return holder.ok ? &holder.program : nullptr;
  }}
}};
RFLINK_REGISTER_DECODER({class_name});
}} }}  // namespace esphome::rflink_universal
'''
        manifest = {
            "schema": 2, "id": pack_id, "name": d["name"] + " (review candidate)",
            "description": "Generated RX candidate from a tested user recipe; disabled until native review.",
            "bands": [f"board radio {d['radio']}"], "modes": [d["kind"]], "device_types": ["remote"],
            "pairing": False, "tx": "disabled", "maturity": "development", "difficulty": "advanced",
            "identity_note": "Identity and action extraction are inherited from the reviewed user VM recipe.",
            "host_test": False, "enabled": False,
            "capabilities": [{
                "protocol": d["id"], "rx_actions": d["actions"], "fields": [],
                "transmit": {"encoder": "none", "status": "disabled", "actions": [],
                             "default_enabled": False,
                             "note": "TX remains in the user/firmware recipe until separate hardware review."},
                "learn": {"category": "development", "band": f"Board radio {d['radio']}",
                          "instruction": "Replay the recorded positive and negative samples before enabling this candidate.",
                          "ha_result": "Same received actions as the reviewed user recipe.",
                          "warning": "Generated candidate: inspect and test before moving into the official decoder catalog."}
            }]
        }
        (target / "decoder.cpp").write_text(source, encoding="utf-8")
        (target / "manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        (target / "recipe.json").write_text(json.dumps(d, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        (target / "README.md").write_text(
            f"# {d['name']} candidate\n\nGenerated from `{d['id']}`. This pack is **disabled** and RX-only. "
            "Its decoder still runs the bounded user VM recipe so behavior remains comparable during review. "
            "Only after replay, host tests and hardware verification should it be rewritten/optimized as a normal native decoder. "
            "TX is intentionally not registered here.\n", encoding="utf-8")
        written.append(target)
    return written


def host_test(header, decoders):
    with tempfile.TemporaryDirectory(prefix="rflink-promotion-") as folder:
        temp = Path(folder)
        (temp / "firmware_user_recipes.h").write_text(header, encoding="utf-8")
        test = '''#include <cassert>
#include "firmware_user_recipes.h"
using namespace esphome::rflink_universal;
int main() {
  UserLibrary library;
  assert(load_firmware_user_recipes(library));
  for (const auto &d : library.decoders()) {
    std::string error;
    assert(d.enabled && library.replay(d,error));
    for (const auto &sample : d.samples) {
      RFFrame frame; frame.channel=sample.channel; frame.kind=sample.kind;
      std::vector<int32_t> raw=sample.raw; std::vector<uint8_t> packet=sample.packet;
      if (frame.kind==FrameKind::RAW) raw.push_back(1000); else packet.push_back(0);
      frame.raw=&raw; frame.packet=&packet;
      DecodeResult result;
      assert(!user_vm::receive(d.rx,d.id,frame,result,error));
    }
  }
}
'''
        (temp / "test.cpp").write_text(test, encoding="utf-8")
        binary = temp / "verify"
        subprocess.run(["g++", "-std=c++17", "-Wall", "-Wextra", "-Werror", "-O0",
                        "-I", str(temp), "-I", str(COMPONENT), str(temp / "test.cpp"),
                        str(COMPONENT / "user_vm.cpp"), str(COMPONENT / "user_library.cpp"),
                        "-o", str(binary)], check=True, capture_output=True, text=True)
        subprocess.run([str(binary)], check=True, capture_output=True, text=True)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("backup", type=Path)
    mode = ap.add_mutually_exclusive_group(required=True)
    mode.add_argument("--check", action="store_true", help="evaluate, compile and replay; do not modify firmware")
    mode.add_argument("--merge", action="store_true", help="merge validated recipes into firmware source")
    ap.add_argument("--replace", action="append", default=[], metavar="ID", help="explicitly replace one existing firmware recipe")
    ap.add_argument("--out-dir", type=Path, default=COMPONENT, help="firmware component directory, useful for dry-run integration tests")
    ap.add_argument("--candidate-out", type=Path, help="also generate disabled RX-only decoder-pack candidates for review")
    args = ap.parse_args()
    if not args.out_dir.is_dir():
        raise ValueError("output directory does not exist")
    manifest = args.out_dir / MANIFEST.name
    header_path = args.out_dir / HEADER.name
    incoming, profiles = parse_backup(args.backup)
    old = json.loads(manifest.read_text(encoding="utf-8")) if manifest.exists() else {"decoders": [], "profiles": []}
    merged = {item["id"]: item for item in old["decoders"]}
    for item in incoming:
        previous = merged.get(item["id"])
        if previous and fingerprint(previous) != fingerprint(item) and item["id"] not in args.replace:
            raise ValueError("firmware recipe changed: " + item["id"] + "; inspect and use --replace " + item["id"])
        merged[item["id"]] = item
    all_profiles = {p["id"]: p for p in old["profiles"]}
    for p in profiles:
        if p["id"] in all_profiles and all_profiles[p["id"]] != p and p["protocol"] not in args.replace:
            raise ValueError("profile changed: " + p["id"] + "; explicitly replace its decoder")
        all_profiles[p["id"]] = p
    header = generate(list(merged.values()), list(all_profiles.values()))
    host_test(header, incoming)
    print(f"PASS: {len(incoming)} new/updated decoders, {len(profiles)} profiles; compile, RX positive/negative, TX preview, and persistence-style replay")
    print("RF hardware, rolling-code pairing and target acknowledgements still need physical tests.")
    if args.candidate_out:
        generated_candidates = write_candidate_packs(args.candidate_out, incoming)
        print("Review candidates:", ", ".join(str(path) for path in generated_candidates))
    if args.merge:
        # Never alter either file until schema and actual C++ integration pass.
        generated = {"schema": 1, "decoders": list(merged.values()), "profiles": list(all_profiles.values())}
        with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8", dir=args.out_dir, delete=False) as fh:
            fh.write(header)
            header_tmp = Path(fh.name)
        with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8", dir=args.out_dir, delete=False) as fh:
            json.dump(generated, fh, ensure_ascii=False, indent=2)
            manifest_tmp = Path(fh.name)
        header_tmp.replace(header_path)
        manifest_tmp.replace(manifest)
        print("Merged firmware recipes:", header_path)


if __name__ == "__main__":
    try:
        main()
    except (ValueError, OSError, subprocess.CalledProcessError) as exc:
        raise SystemExit("FAILED: " + (exc.stderr if isinstance(exc, subprocess.CalledProcessError) and exc.stderr else str(exc)))
