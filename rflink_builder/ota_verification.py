"""Evidence-based verification of a native RFLink ESP32 OTA reboot.

An HTTP 200 from /api/ota only confirms that the *previous* application
accepted the image. A successful update additionally requires a different
boot slot, the selected app's exact embedded version and ESP-IDF's health
check marking the running OTA image VALID. Never infer these from an upload
percentage, uptime alone or a matching board pinout.
"""
from __future__ import annotations
import time


class OTAUnverified(RuntimeError):
    """The image was accepted but the new running firmware was not proven."""


class OTARollback(RuntimeError):
    """Evidence indicates the device returned to an earlier boot slot."""


def _slot(value):
    if isinstance(value, int) and not isinstance(value, bool) and 0 <= value <= 255:
        return value
    return None


def confirm_ota_restart(read_status, before: dict, expected_version: str, report,
                        *, timeout=90, poll_every=2, clock=time.monotonic, sleep=time.sleep):
    """Poll the same identified board after the OTA reply, no credentials.

    Report callback records phase, message, and an optional observation.
    Only a valid new OTA partition is a successful result.
    """
    original_slot = _slot(before.get("ota_slot"))
    if original_slot is None or not expected_version:
        raise OTAUnverified("RFLink did not supply a usable initial OTA slot or target version")
    original_uptime = before.get("uptime_s")
    identity = before.get("device_id")
    board = (before.get("board_id"), before.get("board_hash"))
    began = clock()
    changed_slot_seen = False
    last_phase = None
    attempts = 0
    last_error = ""
    while clock()-began < timeout:
        sleep(min(poll_every, max(0, timeout-(clock()-began))))
        attempts += 1
        try:
            status = read_status()
        except (OSError, RuntimeError, ValueError) as exc:
            last_error = str(exc)[:140]
            if last_phase != "reconnecting":
                report("reconnecting", "Waiting for the ESP32 to restart and reconnect to Wi-Fi…", None)
                last_phase = "reconnecting"
            continue
        if not isinstance(status, dict):
            raise OTAUnverified("RFLink returned an invalid status after reboot")
        if ((status.get("board_id"), status.get("board_hash")) != board or
            (identity and status.get("device_id") != identity)):
            raise OTAUnverified("A different RFLink responded at the saved address; cannot verify the update")
        slot = _slot(status.get("ota_slot"))
        current = status.get("firmware")
        state = status.get("ota_state")
        observation = {"firmware":current,"slot":slot,"ota_state":state,
                       "uptime_s":status.get("uptime_s")}
        if slot is None:
            raise OTAUnverified("RFLink no longer reports its active OTA boot slot")
        if slot != original_slot:
            changed_slot_seen = True
            if current != expected_version:
                raise OTAUnverified(
                    f"Different image booted in the new OTA slot: {current!r}, expected {expected_version!r}")
            # ESP_OTA_IMG_VALID = 2; PENDING_VERIFY = 1.
            if state == 2:
                report("verified", "New firmware booted and ESP-IDF marked its OTA health VALID.", observation)
                return observation
            if state in (3,4):
                raise OTARollback("The new image was rejected by ESP-IDF OTA health checks")
            if last_phase != "validating":
                report("validating",
                       "New firmware and boot slot detected. Waiting for the ESP32's 10-second health check…",
                       observation)
                last_phase = "validating"
        else:
            if changed_slot_seen:
                raise OTARollback("ESP32 returned to its previous OTA slot after starting the update: rollback suspected")
            restarted=(isinstance(original_uptime,(int,float)) and
                       isinstance(status.get("uptime_s"),(int,float)) and
                       status["uptime_s"]+2 < original_uptime)
            if restarted and clock()-began>=12:
                raise OTARollback("ESP32 restarted into the previous OTA slot; update likely rolled back")
            if last_phase != "reconnecting":
                report("reconnecting", "Still observing the previous boot slot; waiting for the OTA reboot…",
                       observation)
                last_phase = "reconnecting"
            elif attempts % 8 == 0:
                report("reconnecting", "Still waiting for new firmware; last observed slot unchanged.",
                       observation)
    extra = f" Last network error: {last_error}" if last_error else ""
    raise OTAUnverified(
        "OTA image was accepted but the new boot slot, firmware version and VALID health state "
        "could not all be confirmed. Inspect RFLink diagnosis or serial boot logs."+extra)
