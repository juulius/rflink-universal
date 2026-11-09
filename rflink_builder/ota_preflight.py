"""Safe, deterministic device status snapshot captured before a network OTA.

Deliberate allowlist: never dump unvalidated device JSON, saved credentials,
paired RF device IDs, keys, logs, messages, or arbitrary future API fields.
Unknown old-firmware fields are labeled unavailable rather than guessed.
"""
from __future__ import annotations
import re


def _text(value, max_length=80):
    if not isinstance(value,str):
        return "unknown"
    safe=re.sub(r"[^a-zA-Z0-9 _.:+@/-]", "_", value[:max_length]).strip()
    return safe if safe else "unknown"


def _number(value, minimum=0, maximum=2**53-1):
    if type(value) is int and minimum<=value<=maximum:
        return str(value)
    return "unknown"


def _flag(value):
    return "YES" if value is True else "NO" if value is False else "unknown"


def _array(source, key, kind="number", count=3):
    value=source.get(key)
    if not isinstance(value,list) or len(value)!=count:
        return "unknown"
    items=[]
    for v in value:
        if kind=="bool":
            items.append(_flag(v))
        else:
            items.append(_number(v,-100000,2**32-1))
    return " / ".join(items)


def _pins(source,key,groups=False):
    value=source.get(key)
    if groups:
        if not isinstance(value,list) or len(value)!=3:
            return "unknown"
        return " | ".join(
            "("+", ".join(_number(v,0,48) for v in group)+")"
            if isinstance(group,list) and len(group)==3 else "unknown"
            for group in value)
    if not isinstance(value,list) or len(value)!=3:
        return "unknown"
    return " / ".join(_number(v,0,48) for v in value)


def format_ota_preflight_status(status:dict, ip:str)->str:
    """Report every current *safe and relevant* /api/status field before OTA."""
    if not isinstance(status,dict):
        status={}
    def n(key,minimum=0,maximum=2**53-1):
        return _number(status.get(key),minimum,maximum)
    def f(key):
        return _flag(status.get(key))
    def t(key,limit=80):
        return _text(status.get(key),limit)
    lines=[
        "       — Live RFLink status (read from ESP32 before OTA) —",
        "       Network / device:",
        "         IP: "+_text(ip,45)+" · physical ID: "+t("device_id",24),
        "         Firmware: "+t("firmware",64)+
            " · channel: "+t("release_channel",24)+
            " · build: "+n("release_build",0,65535),
        "         Board: "+t("board_id",70)+" · profile: "+t("board_hash",24),
        "         Wi-Fi: "+f("wifi_connected")+" · MQTT: "+f("mqtt_connected")+
            " · uptime: "+n("uptime_s")+" s",
        "         Free heap: "+n("free_heap_bytes")+" bytes",
        "       OTA / flash:",
        "         Current slot: "+n("ota_slot",0,255)+
            " · image state: "+n("ota_state",-1,10)+
            " · busy: "+f("ota_busy"),
        "         Slot count: "+n("ota_slot_count",0,2)+
            " · inactive target: "+n("ota_target_slot",-1,255)+
            " · safe: "+f("ota_update_safe"),
        "         Running offset: "+(
            hex(status["ota_running_address"]) if type(status.get("ota_running_address")) is int
            and 0<=status["ota_running_address"]<=0xffffff else "unknown")+
            " · target offset: "+(
            hex(status["ota_target_address"]) if type(status.get("ota_target_address")) is int
            and 0<=status["ota_target_address"]<=0xffffff else "unknown"),
        "         Layout check: "+t("ota_layout_issue",48),
        "       Hardware / radio:",
        "         SPI bus ready: "+f("spi_bus_ready")+
            " · radios initialized: "+f("radio_initialized")+
            " · application ready: "+f("startup_complete"),
        "         SPI clock: "+n("spi_clock_hz",0,100000000)+" Hz"+
            " · pins [CLK/MOSI/MISO]: "+_pins(status,"spi_pins"),
        "         Radio pins [CS/GDO0/GDO2]: "+_pins(status,"radio_pins",groups=True),
        "         CC1101 detected [433.42/433.92/868.30]: "+_array(status,"cc1101_present","bool"),
        "         SPI ready timeouts: "+_array(status,"spi_ready_timeouts"),
        "         SPI transfer errors: "+_array(status,"spi_transfer_errors"),
        "         Radio operations: "+_array(status,"radio_operation"),
        "         RX worker cycles: "+n("rx_worker_cycles"),
        "         433.42 RX: listening "+f("rx43342_listening")+
            " · frames "+n("rx43342_frames")+
            " · RMT stage/code "+n("rx43342_rmt_error_stage",0,255)+
            "/"+n("rx43342_rmt_error_code",-65535,65535),
        "         433.42 start: stage/code "+n("rx43342_start_error_stage",0,255)+
            "/"+n("rx43342_start_error_code",-65535,65535)+
            " · register "+n("rx43342_start_error_reg",-1,255)+
            " · expected/observed "+n("rx43342_start_expected",-1,255)+
            "/"+n("rx43342_start_observed",-1,255)+
            " · failures "+n("rx43342_start_failures"),
        "         433.42 RMT errors: "+n("rx43342_rmt_errors"),
        "         433.92 RX: listening "+f("rx43392_listening")+
            " · frames "+n("rx43392_frames")+
            " · RMT stage/code "+n("rx43392_rmt_error_stage",0,255)+
            "/"+n("rx43392_rmt_error_code",-65535,65535),
        "         433.92 start: stage/code "+n("rx43392_start_error_stage",0,255)+
            "/"+n("rx43392_start_error_code",-65535,65535)+
            " · register "+n("rx43392_start_error_reg",-1,255)+
            " · expected/observed "+n("rx43392_start_expected",-1,255)+
            "/"+n("rx43392_start_observed",-1,255)+
            " · failures "+n("rx43392_start_failures"),
        "         433.92 RMT errors: "+n("rx43392_rmt_errors"),
        "         868.30 RX: listening "+f("rx868_listening")+
            " · packets "+n("rx868_packets")+
            " · FIFO overflows "+n("rx868_fifo_overflows")+
            " · SPI errors "+n("rx868_spi_errors")+
            " · recoveries "+n("rx868_recoveries")+
            " · idle rearms "+n("rx868_idle_rearms"),
        "       Decoder / TX:",
        "         Itho "+n("decode_itho")+" · Ramses "+n("decode_ramses")+
            " · unknown "+n("decode_unknown")+
            " · overlaps "+n("decode_overlap"),
        "         Pulse decodes [433.42/433.92]: "+n("decode_pulses_43342")+
            "/"+n("decode_pulses_43392"),
        "         RX implemented: "+f("rx_implemented")+
            " · 868 packet diagnostic: "+f("rx868_packet_diagnostic"),
        "         TX implemented: "+f("tx_implemented")+
            " · TX driver present: "+f("tx_lab_driver_present")+
            " · last TX stage: "+n("tx_lab_stage",0,255),
        "       Note: unknown means unsupported or missing in the running firmware.",
        "       No secrets, management keys or RF payloads are included.",
    ]
    return "\n".join(lines)+"\n"


def format_ota_saved_boot_report(report:dict)->str:
    """Optional sanitized persisted boot record, never required for OTA."""
    if not isinstance(report,dict):
        return ""
    out=[
        "       — Stored boot diagnosis (pre-update) —",
        "         Last reset: "+_text(report.get("last_reset"),48),
        "         Boot history saved: "+_flag(report.get("history_saved"))+
            " · phases saved: "+_flag(report.get("phase_history_saved")),
    ]
    phases=report.get("boot_phases")
    if isinstance(phases,list):
        for phase in phases[:3]:
            if not isinstance(phase,dict):
                continue
            out.append(
                "         Boot #"+_number(phase.get("boot"),1,2**32-1)+
                " · "+_text(phase.get("stage"),32)+
                " · complete "+_flag(phase.get("startup_complete")))
    health=report.get("ota_health")
    if isinstance(health,dict):
        out+=["         Previous OTA health: "+_text(health.get("outcome"),30)+
              " · firmware "+_text(health.get("firmware"),36)+
              " · SPI "+_flag(health.get("spi_ok"))+
              " · web "+_flag(health.get("web_ok"))+
              " · worker "+_flag(health.get("worker_ok")),
              "         Previous OTA RX cycles "+_number(health.get("cycles"))+
              " · active "+_number(health.get("active_receivers"),0,3)+
              " · responsive radios "+_number(health.get("responsive_radios"),0,3)]
    return "\n".join(out)+"\n"
