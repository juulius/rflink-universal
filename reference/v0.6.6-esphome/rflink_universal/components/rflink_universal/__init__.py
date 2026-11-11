import json
import inspect
import re
import shutil
from pathlib import Path

import esphome.codegen as cg
from esphome import pins
import esphome.config_validation as cv
from esphome.components import cc1101, remote_base, remote_receiver, remote_transmitter
from esphome.components.logger import request_log_listener
from esphome.const import CONF_ID, CONF_NAME
from esphome.core import CORE
from esphome.helpers import copy_file_if_changed, walk_files, write_file_if_changed
from .manifest_schema import load_manifest, generate_header
from .radio_transport_manifest import generate_radio_header
from .radio_contract_v2 import generate_contract_header

DEPENDENCIES = ["remote_receiver", "network"]
AUTO_LOAD = ["web_server_base"]
MULTI_CONF = False

CONF_CHANNELS = "channels"
CONF_FREQUENCY = "frequency"
CONF_RECEIVER_ID = "receiver_id"
CONF_DEDUPE_WINDOW = "dedupe_window"
CONF_CANDIDATE_THRESHOLD = "candidate_threshold"
CONF_MODE = "mode"
CONF_WEB_UI = "web_ui"
CONF_DECODER_DIRECTORY = "decoder_directory"
CONF_HA_NODE = "ha_node"
CONF_NEXA_TRANSMITTER = "nexa_transmitter"
CONF_TX_CONTROLS_ENABLED = "tx_controls_enabled"
CONF_PACKET_TRANSMITTER = "packet_transmitter"
CONF_RAMSES_SPI_LAB = "ramses_spi_lab"
CONF_RAMSES_SPI_UNSYNCED = "ramses_spi_unsynced"
CONF_RAMSES_RADIO_LAB = "ramses_radio_lab"
CONF_DECODER_AUTO_SCAN_LAB = "decoder_auto_scan_lab"
CONF_RAMSES_UART_GDO2 = "ramses_uart_gdo2_pin"
CONF_GDO_SERIAL_TX_LAB = "gdo_serial_tx_lab"
CONF_RAMSES_TX_PROBE_LAB = "ramses_tx_probe_lab"
CONF_GDO_SERIAL_TX_GPIO = "gdo_serial_tx_gpio"
CONF_RADIO_GUARD_ID = "radio_guard_id"
CONF_RADIO_CS_PINS = "radio_cs_pins"
CONF_RADIO_GDO_PINS = "radio_gdo_pins"
CONF_RADIO_SCK_PIN = "radio_sck_pin"
CONF_RADIO_MOSI_PIN = "radio_mosi_pin"
CONF_RADIO_MISO_PIN = "radio_miso_pin"
CONF_RADIO_STARTUP_DELAY = "radio_startup_delay"
CONF_RADIO_READY_TIMEOUT = "radio_ready_timeout"
CONF_RADIO_READY_RETRIES = "radio_ready_retries"
CONF_RADIO_POST_GUARD_DELAY = "radio_post_guard_delay"

rflink_ns = cg.esphome_ns.namespace("rflink_universal")
RFLinkUniversal = rflink_ns.class_("RFLinkUniversal", cg.Component)
RadioBootGuard = rflink_ns.class_("RadioBootGuard", cg.Component)

_PACK_ID_RE = re.compile(r"^[a-z][a-z0-9_]*$")
_REQUIRED_MANIFEST_KEYS = {
    "schema",
    "id",
    "name",
    "description",
    "bands",
    "modes",
    "device_types",
    "pairing",
    "tx",
    "maturity",
    "difficulty",
    "identity_note",
}


def _discover_decoder_packs(decoder_dir: Path):
    """Return validated decoder packs from one user-facing directory.

    Adding a decoder means adding one folder with decoder.cpp + manifest.json.
    Nothing in the core, registry, web UI or constants files needs editing.
    Directories beginning with '_' are templates/documentation and ignored.
    """
    if not decoder_dir.is_dir():
        raise cv.Invalid(f"RFLink decoder directory does not exist: {decoder_dir}")
    packs = []
    for folder in sorted(p for p in decoder_dir.iterdir() if p.is_dir() and not p.name.startswith("_")):
        source = folder / "decoder.cpp"
        manifest_path = folder / "manifest.json"
        if not source.is_file() or not manifest_path.is_file():
            raise cv.Invalid(
                f"RFLink decoder pack '{folder.name}' must contain decoder.cpp and manifest.json"
            )
        try:
            manifest = load_manifest(manifest_path)
        except (OSError, ValueError, KeyError, TypeError) as err:
            raise cv.Invalid(f"Invalid manifest for RFLink decoder pack '{folder.name}': {err}") from err
        missing = sorted(_REQUIRED_MANIFEST_KEYS - set(manifest))
        if missing:
            raise cv.Invalid(
                f"RFLink decoder pack '{folder.name}' manifest is missing: {', '.join(missing)}"
            )
        if manifest["schema"] != 2:
            raise cv.Invalid(f"RFLink decoder pack '{folder.name}' requires schema 2")
        if manifest["id"] != folder.name:
            raise cv.Invalid(
                f"RFLink decoder folder '{folder.name}' must match manifest id '{manifest['id']}'"
            )
        if not _PACK_ID_RE.fullmatch(folder.name):
            raise cv.Invalid(
                f"RFLink decoder id '{folder.name}' may only contain lowercase letters, numbers and underscores"
            )
        if manifest.get("enabled", True):
            packs.append((folder, manifest))
    return packs


def _copy_decoder_packs_to_build(packs):
    """Copy enabled decoder source trees into src/rflink_decoders.

    This path intentionally sits outside src/esphome so ESPHome's component
    source synchronizer will not delete it after code generation. Both the
    PlatformIO and native ESP-IDF builds compile .cpp files below src
    recursively. This keeps decoder packs physically separate from the core.
    """
    build_root = CORE.relative_src_path("rflink_decoders")
    # Static constructors alone do NOT pull an unreferenced decoder .o from
    # an ESP-IDF archive: generate strong linker references for every enabled
    # decoder. When a source object is not compiled, fail at link time instead
    # of silently booting with `0 decoder(s) registered`.
    anchors = []
    tx_anchors = []
    build_root.mkdir(parents=True, exist_ok=True)
    # A formerly enabled decoder must not remain in the generated build after
    # a pack is disabled/removed (otherwise static constructors register ghosts).
    enabled_names = {folder.name for folder, _manifest in packs}
    for stale_dir in build_root.iterdir():
        if stale_dir.is_dir() and stale_dir.name not in enabled_names:
            shutil.rmtree(stale_dir)
    for folder, _manifest in packs:
        source = (folder / "decoder.cpp").read_text(encoding="utf-8")
        matches = re.findall(r"\bRFLINK_REGISTER_DECODER\(\s*([A-Za-z_][A-Za-z_0-9]*)\s*\)", source)
        if not matches:
            raise cv.Invalid(f"Enabled decoder pack '{folder.name}' contains no registered decoder")
        anchors.extend(matches)
        tx_file = folder / "tx_encoder.cpp"
        tx_source = tx_file.read_text(encoding="utf-8") if tx_file.exists() else ""
        tx_matches = re.findall(r"\bRFLINK_REGISTER_TX_ENCODER\(\s*([A-Za-z_][A-Za-z_0-9]*)\s*\)", tx_source)
        tx_anchors.extend(tx_matches)
        # Every manifest claim to TX must be backed by a compiled encoder.
        claimed = sum(c["transmit"]["encoder"] != "none" for c in _manifest["capabilities"])
        if claimed != len(tx_matches):
            raise cv.Invalid(f"Decoder pack '{folder.name}' claims {claimed} TX encoders but registers {len(tx_matches)}")
    if len(tx_anchors) != len(set(tx_anchors)):
        raise cv.Invalid("Duplicate TX encoder class name across enabled decoder packs")
    if len(anchors) != len(set(anchors)):
        raise cv.Invalid("Duplicate decoder class name across enabled decoder packs")
    anchor_content = ("// AUTOGENERATED; do not edit. Decoder linker anchors.\n"
                      "#pragma once\n\n" +
                      "".join(f'extern "C" ::esphome::rflink_universal::RFDecoder *rflink_anchor_{name}();\n' for name in anchors) +
                      "\ninline void rflink_generated_force_link_decoders() {\n" +
                      "".join(f"  ::esphome::rflink_universal::register_decoder_once(rflink_anchor_{name}());\n" for name in anchors) +
                      "}\n")
    # Native developer and Windows release packaging never need Python here;
    # ESPHome itself executes this code during firmware generation.
    from esphome.helpers import write_file_if_changed
    write_file_if_changed(build_root / "decoder_anchors_generated.h", anchor_content)
    tx_content = ("// AUTOGENERATED; do not edit. Strong TX encoder linker anchors.\n#pragma once\n" +
                  "".join(f'extern "C" ::esphome::rflink_universal::tx_engine::Encoder *rflink_tx_anchor_{name}();\n' for name in tx_anchors) +
                  "\ninline void rflink_generated_force_link_tx_encoders() {\n" +
                  "".join(f"  ::esphome::rflink_universal::tx_engine::register_once(rflink_tx_anchor_{name}());\n" for name in tx_anchors) +
                  "}\n")
    write_file_if_changed(build_root / "tx_encoder_anchors_generated.h", tx_content)
    for folder, _manifest in packs:
        for src in walk_files(folder):
            src = Path(src)
            if src.suffix not in {".cpp", ".h", ".hpp", ".inc", ".tcc"}:
                continue
            rel = src.relative_to(folder.parent)
            dst = build_root / rel
            copy_file_if_changed(src, dst)
        # Also remove files dropped from a still-enabled decoder pack.
        expected = {Path(f).relative_to(folder) for f in walk_files(folder)
                    if Path(f).suffix in {".cpp", ".h", ".hpp", ".inc", ".tcc"}}
        generated_pack_dir = build_root / folder.name
        for old in generated_pack_dir.rglob("*"):
            if old.is_file() and old.relative_to(generated_pack_dir) not in expected:
                old.unlink()
    return len(anchors)


def _channel_schema(value):
    schema = cv.Schema({
        cv.Required(CONF_NAME): cv.string_strict,
        cv.Required(CONF_FREQUENCY): cv.int_range(min=100_000_000, max=1_000_000_000),
        cv.Optional(CONF_MODE, default="raw"): cv.one_of("raw", "packet", lower=True),
        cv.Optional(CONF_RECEIVER_ID): cv.use_id(remote_receiver.RemoteReceiverComponent),
    })
    value = schema(value)
    if value[CONF_MODE] == "raw" and CONF_RECEIVER_ID not in value:
        raise cv.Invalid("raw channel requires receiver_id")
    return value


def _raw_esp32_gpio_number(value):
    """Validate a raw ESP32 GPIO number without registering/claiming the pin in ESPHome.

    The boot guard intentionally touches the same physical CS/MISO lines before the
    normal SPI/CC1101 components are set up. Using ESPHome pin schemas here would
    claim those pins a second time and trigger "Pin X is used in multiple places".
    """
    if isinstance(value, bool):
        raise cv.Invalid("GPIO number must not be a boolean")
    if isinstance(value, int):
        number = value
    elif isinstance(value, str):
        match = re.fullmatch(r"(?:GPIO)?([0-9]+)", value.strip(), re.IGNORECASE)
        if not match:
            raise cv.Invalid(f"Invalid ESP32 GPIO value: {value!r}")
        number = int(match.group(1))
    else:
        raise cv.Invalid(f"Invalid ESP32 GPIO value: {value!r}")
    if not (0 <= number <= 21 or 26 <= number <= 48):
        raise cv.Invalid(f"ESP32-S3 GPIO {number} does not exist")
    return number


CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(RFLinkUniversal),
    cv.GenerateID(CONF_RADIO_GUARD_ID): cv.declare_id(RadioBootGuard),
    # Raw GPIO numbers on purpose: these are aliases of the real SPI/CC1101 pins,
    # not additional ESPHome pin owners. This avoids duplicate-pin validation.
    cv.Required(CONF_RADIO_CS_PINS): cv.All(
        cv.ensure_list(_raw_esp32_gpio_number), cv.Length(min=1, max=3)
    ),
    cv.Optional(CONF_RADIO_GDO_PINS, default=[]): cv.All(
        cv.ensure_list(_raw_esp32_gpio_number), cv.Length(max=6)
    ),
    cv.Required(CONF_RADIO_SCK_PIN): _raw_esp32_gpio_number,
    cv.Required(CONF_RADIO_MOSI_PIN): _raw_esp32_gpio_number,
    cv.Required(CONF_RADIO_MISO_PIN): _raw_esp32_gpio_number,
    cv.Optional(CONF_RADIO_STARTUP_DELAY, default="200ms"): cv.positive_time_period_milliseconds,
    cv.Optional(CONF_RADIO_READY_TIMEOUT, default="10ms"): cv.positive_time_period_microseconds,
    cv.Optional(CONF_RADIO_READY_RETRIES, default=5): cv.int_range(min=1, max=8),
    cv.Optional(CONF_RADIO_POST_GUARD_DELAY, default="25ms"): cv.positive_time_period_milliseconds,
    cv.Required(CONF_CHANNELS): cv.All(cv.ensure_list(_channel_schema), cv.Length(min=1, max=3)),
    cv.Optional(CONF_DEDUPE_WINDOW, default="80ms"): cv.positive_time_period_milliseconds,
    cv.Optional(CONF_CANDIDATE_THRESHOLD, default=3): cv.int_range(min=1, max=20),
    cv.Optional(CONF_WEB_UI, default=True): cv.boolean,
    cv.Optional(CONF_HA_NODE, default="rflink-universal"): cv.string_strict,
    cv.Optional(CONF_NEXA_TRANSMITTER): cv.use_id(remote_transmitter.RemoteTransmitterComponent),
    cv.Optional("raw_transmitter_43342"): cv.use_id(remote_transmitter.RemoteTransmitterComponent),
    cv.Optional("raw_transmitter_43392"): cv.use_id(remote_transmitter.RemoteTransmitterComponent),
    cv.Optional(CONF_TX_CONTROLS_ENABLED, default=False): cv.boolean,
    cv.Optional(CONF_PACKET_TRANSMITTER): cv.use_id(cc1101.CC1101Component),
    cv.Optional(CONF_RAMSES_SPI_LAB, default=False): cv.boolean,
    cv.Optional(CONF_RAMSES_SPI_UNSYNCED, default=False): cv.boolean,
    cv.Optional(CONF_RAMSES_RADIO_LAB, default=False): cv.boolean,
    cv.Optional(CONF_DECODER_AUTO_SCAN_LAB, default=False): cv.boolean,
    cv.Optional(CONF_RAMSES_UART_GDO2): _raw_esp32_gpio_number,
    cv.Optional(CONF_GDO_SERIAL_TX_LAB, default=False): cv.boolean,
    cv.Optional(CONF_RAMSES_TX_PROBE_LAB, default=False): cv.boolean,
    cv.Optional(CONF_GDO_SERIAL_TX_GPIO): _raw_esp32_gpio_number,
    cv.Optional(CONF_DECODER_DIRECTORY, default="decoders"): cv.string_strict,
}).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    # ESPHome 2026.10 Logger uses a fixed-capacity StaticVector. Reserve our
    # crash-report callback slot; API streaming reserves its own slot.
    if CORE.is_esp32:
        request_log_listener()
    if config[CONF_RAMSES_TX_PROBE_LAB] and not config[CONF_GDO_SERIAL_TX_LAB]:
        raise cv.Invalid("ramses_tx_probe_lab requires gdo_serial_tx_lab")
    if config[CONF_GDO_SERIAL_TX_LAB]:
        if CONF_PACKET_TRANSMITTER not in config:
            raise cv.Invalid("gdo_serial_tx_lab requires packet_transmitter (existing 868 MHz SPI radio owner)")
        if CONF_GDO_SERIAL_TX_GPIO not in config:
            raise cv.Invalid("gdo_serial_tx_lab requires gdo_serial_tx_gpio (868 MHz GDO0 physical PCB pin)")
        if config[CONF_GDO_SERIAL_TX_GPIO] in {45,46}:
            raise cv.Invalid("GPIO45/46 are not permitted for GDO serial TX")
        pins = config[CONF_RADIO_GDO_PINS]
        if len(pins) < 6 or pins[4] != config[CONF_GDO_SERIAL_TX_GPIO]:
            raise cv.Invalid("GDO serial TX pin must match the fifth entry in radio_gdo_pins (radio 3 GDO0)")
    decoder_dir = CORE.relative_config_path(config[CONF_DECODER_DIRECTORY])
    packs = _discover_decoder_packs(decoder_dir)
    expected = _copy_decoder_packs_to_build(packs)
    # ESPHome 2026.10+ compiles remote_base protocols only when requested by
    # code generation. The enabled ESPHome built-in decoder pack invokes these
    # directly from C++ (including its TX encoders), so the Python YAML parser
    # cannot discover the dependency on its own. Older ESPHome compiles all
    # protocols and has no request_protocol() helper.
    if any(folder.name == "esphome_builtin" for folder, _manifest in packs):
        if hasattr(remote_base, "request_protocol"):
            for protocol in ("nexa", "byronsx", "dooya", "keeloq", "rc_switch"):
                remote_base.request_protocol(protocol)
    # Generate from the ACTUAL enabled decoder manifests on every ESPHome build.
    # This is independent from the checked-in fallback used by host tests.
    try:
        catalog = generate_header(packs)
    except (ValueError, TypeError, KeyError) as err:
        raise cv.Invalid(f"RFLink capability catalog error: {err}") from err
    write_file_if_changed(CORE.relative_src_path("rflink_decoders/capability_catalog_generated.h"), catalog)
    write_file_if_changed(CORE.relative_src_path("rflink_decoders/radio_transport_catalog_generated.h"), generate_radio_header(packs))
    write_file_if_changed(CORE.relative_src_path("rflink_decoders/radio_v2_catalog_generated.h"), generate_contract_header(packs))
    cg.add_define("RFLINK_DECODER_PACK_COUNT", len(packs))
    cg.add_define("RFLINK_DECODER_EXPECTED_COUNT", expected)
    cg.add_define("RFLINK_TX_ENCODER_EXPECTED_COUNT", sum(
        sum(cap["transmit"]["encoder"] != "none" for cap in manifest["capabilities"])
        for _, manifest in packs))

    guard = cg.new_Pvariable(config[CONF_RADIO_GUARD_ID])
    await cg.register_component(guard, config)
    for cs_pin in config[CONF_RADIO_CS_PINS]:
        cg.add(guard.add_cs_pin(cs_pin))
    for gdo_pin in config[CONF_RADIO_GDO_PINS]:
        cg.add(guard.add_gdo_pin(gdo_pin))
    cg.add(guard.set_sck_pin(config[CONF_RADIO_SCK_PIN]))
    cg.add(guard.set_mosi_pin(config[CONF_RADIO_MOSI_PIN]))
    cg.add(guard.set_miso_pin(config[CONF_RADIO_MISO_PIN]))
    cg.add(guard.set_startup_delay_ms(config[CONF_RADIO_STARTUP_DELAY].total_milliseconds))
    cg.add(guard.set_ready_timeout_us(config[CONF_RADIO_READY_TIMEOUT].total_microseconds))
    cg.add(guard.set_ready_retries(config[CONF_RADIO_READY_RETRIES]))
    cg.add(guard.set_post_guard_delay_ms(config[CONF_RADIO_POST_GUARD_DELAY].total_milliseconds))

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_dedupe_window(config[CONF_DEDUPE_WINDOW].total_milliseconds))
    cg.add(var.set_candidate_threshold(config[CONF_CANDIDATE_THRESHOLD]))
    cg.add(var.set_ha_node(config[CONF_HA_NODE]))
    cg.add(var.set_tx_controls_enabled(config[CONF_TX_CONTROLS_ENABLED]))
    if CONF_PACKET_TRANSMITTER in config:
        cg.add(var.set_packet_transmitter(await cg.get_variable(config[CONF_PACKET_TRANSMITTER])))
    cg.add(var.set_ramses_spi_lab(config[CONF_RAMSES_SPI_LAB]))
    cg.add(var.set_ramses_spi_unsynced(config[CONF_RAMSES_SPI_UNSYNCED]))
    cg.add(var.set_ramses_radio_lab(config[CONF_RAMSES_RADIO_LAB]))
    cg.add(var.set_gdo_serial_tx_lab(config[CONF_GDO_SERIAL_TX_LAB]))
    cg.add(var.set_ramses_tx_probe_lab(config[CONF_RAMSES_TX_PROBE_LAB]))
    if CONF_GDO_SERIAL_TX_GPIO in config:
        cg.add(var.set_gdo_serial_tx_gpio(config[CONF_GDO_SERIAL_TX_GPIO]))
    cg.add(var.set_decoder_auto_scan_lab(config[CONF_DECODER_AUTO_SCAN_LAB]))
    if CONF_RAMSES_UART_GDO2 in config:
        cg.add(var.set_ramses_uart_gdo2_pin(config[CONF_RAMSES_UART_GDO2]))
    if CONF_NEXA_TRANSMITTER in config:
        transmitter = await cg.get_variable(config[CONF_NEXA_TRANSMITTER])
        cg.add(var.set_nexa_transmitter(transmitter))
    for index, key in enumerate(("raw_transmitter_43342", "raw_transmitter_43392")):
        if key in config:
            transmitter=await cg.get_variable(config[key])
            cg.add(var.set_raw_transmitter(index, transmitter))
    if config[CONF_WEB_UI]:
        cg.add_define("USE_RFLINK_UNIVERSAL_WEB")

    for index, channel in enumerate(config[CONF_CHANNELS]):
        if channel[CONF_MODE] == "raw":
            receiver = await cg.get_variable(channel[CONF_RECEIVER_ID])
            cg.add(var.set_raw_channel(index, receiver, channel[CONF_FREQUENCY], channel[CONF_NAME]))
            # ESPHome 2026.10+ sizes remote receiver listener slots at codegen.
            # Register our existing per-channel listener in Python rather than setup().
            # ESPHome <= 2026.9 doesn't expose add_listener and keeps the old C++ path.
            if hasattr(remote_base, "add_listener"):
                registration = remote_base.add_listener(receiver, var.get_raw_channel_listener(index))
                if inspect.isawaitable(registration):
                    await registration
        else:
            cg.add(var.set_packet_channel(index, channel[CONF_FREQUENCY], channel[CONF_NAME]))
