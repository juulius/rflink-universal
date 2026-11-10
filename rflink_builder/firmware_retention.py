"""Bounded RFLink firmware package retention for Home Assistant /data.

Only disposable, verified Builder *packages* are candidates for removal. Never
touch project source, device settings, secrets, toolchain or build_workspace.
A device's last OTA-verified package survives while a newer image is being
compiled/flashed, and is replaced only AFTER the new ESP32 reports OTA_VALID.
Browser/host USB command success alone is not evidence of a healthy boot.

For pre-policy device builds with no known verified install, automatic cleanup
is intentionally conservative. An explicit manual cleanup may keep the newest
two packages, which gives the operator one pending plus a recovery copy.
"""
from __future__ import annotations

import json
import os
import re
import shutil
import tempfile
from collections import defaultdict
from pathlib import Path

JOB_RE = re.compile(r"^[0-9a-f]{12}$")
STATE_NAME = "firmware_retention.json"


class FirmwareRetention:
    def __init__(self, data: Path):
        self.data = Path(data)
        self.root = self.data / "releases"
        self.state = self.data / "local" / STATE_NAME

    def _confirmed(self) -> dict[str, str]:
        try:
            if self.state.is_symlink():
                return {}
            data = json.loads(self.state.read_text("utf-8"))
            if not isinstance(data, dict):
                return {}
            entries = data.get("ota_verified", {})
            if isinstance(entries, dict):
                return {k: v for k, v in entries.items()
                        if isinstance(k, str) and isinstance(v, str) and JOB_RE.fullmatch(v)}
        except (FileNotFoundError, OSError, ValueError, TypeError):
            pass
        return {}

    def _write_confirmed(self, verified: dict[str, str]) -> None:
        folder = self.state.parent
        if folder.is_symlink():
            raise OSError("Refusing symlinked firmware retention state directory")
        folder.mkdir(parents=True, exist_ok=True)
        if self.state.is_symlink():
            raise OSError("Refusing symlinked firmware retention state file")
        fd, tmp = tempfile.mkstemp(prefix=".firmware-retention-", dir=folder)
        try:
            os.fchmod(fd, 0o600)
            with os.fdopen(fd, "w", encoding="utf-8") as stream:
                json.dump({"schema": 1, "ota_verified": verified}, stream, indent=2)
                stream.write("\n")
                stream.flush()
                os.fsync(stream.fileno())
            os.replace(tmp, self.state)
        finally:
            if os.path.exists(tmp):
                os.unlink(tmp)

    def _records(self) -> list[dict]:
        if not self.root.exists():
            return []
        if self.root.is_symlink() or not self.root.is_dir():
            raise OSError("Invalid saved firmware directory")
        result = []
        for folder in self.root.iterdir():
            if (folder.is_symlink() or not folder.is_dir()
                    or not JOB_RE.fullmatch(folder.name)):
                continue
            metadata = folder / "build_info.json"
            if metadata.is_symlink() or not metadata.is_file():
                continue
            try:
                if metadata.stat().st_size > 16 * 1024:
                    continue
                info = json.loads(metadata.read_text("utf-8"))
                if not isinstance(info, dict) or info.get("job_id") != folder.name:
                    continue
                device = info.get("device_id")
                if device is not None and (not isinstance(device, str) or not device):
                    continue
                created = info.get("created_at")
                if not isinstance(created, str):
                    created = ""
                # Never prune directories containing symlinks (incl. nested
                # symlinks) or unexpected subdirectories; only Builder outputs
                # are flat. This also makes size accounting non-traversing.
                if any(entry.is_symlink() or not entry.is_file()
                       for entry in folder.iterdir()):
                    continue
                size = sum(entry.stat().st_size for entry in folder.iterdir())
                result.append({"job_id": folder.name, "device_id": device,
                               "created_at": created, "bytes": size,
                               "folder": folder})
            except (OSError, ValueError, TypeError):
                continue
        return result

    def _plan(self, active_job: str | None, manual: bool) -> tuple[list[dict], list[dict], list[dict]]:
        records = self._records()
        groups = defaultdict(list)
        for item in records:
            groups[item["device_id"]].append(item)
        verified = self._confirmed()
        keep = set()
        for device, builds in groups.items():
            builds.sort(key=lambda x: (x["created_at"], x["job_id"]), reverse=True)
            if device is None:
                keep.add(builds[0]["job_id"])  # One general/unassigned package.
                continue
            confirmed = verified.get(device)
            ids = {row["job_id"] for row in builds}
            if confirmed in ids:
                keep.add(confirmed)
                if builds[0]["job_id"] != confirmed:
                    keep.add(builds[0]["job_id"])  # New image awaiting OTA verification.
            elif manual:
                # Only an explicit user-confirmed cleanup may prune pre-policy
                # packages. Preserve two newest while installation is unknown.
                keep.update(row["job_id"] for row in builds[:2])
            else:
                # Unknown installed image (often older browser/USB flashing):
                # do not guess which historical package is the recovery copy.
                keep.update(ids)
        if active_job and JOB_RE.fullmatch(active_job):
            keep.add(active_job)
        retained = [item for item in records if item["job_id"] in keep]
        removed = [item for item in records if item["job_id"] not in keep]
        return records, retained, removed

    def report(self, active_job: str | None = None, busy: bool = False) -> dict:
        records, kept, candidates = self._plan(active_job, manual=True)
        grouped = defaultdict(list)
        for row in records:
            grouped[row["device_id"]].append(row)
        verified = self._confirmed()
        details = []
        for device, builds in sorted(grouped.items(), key=lambda pair: str(pair[0])):
            details.append({
                "device_id": device, "count": len(builds),
                "bytes": sum(row["bytes"] for row in builds),
                "ota_verified": bool(device is not None and
                    verified.get(device) in {row["job_id"] for row in builds}),
                "needs_confirmation": bool(device is not None and
                    verified.get(device) not in {row["job_id"] for row in builds}),
            })
        return {
            "saved_count": len(records),
            "saved_bytes": sum(item["bytes"] for item in records),
            "reclaimable_count": len(candidates),
            "reclaimable_bytes": sum(item["bytes"] for item in candidates),
            "retained_count_after_cleanup": len(kept),
            "busy": bool(busy),
            "devices": details,
            "policy": "1 OTA-verified package per device; 1 pending allowed until verified. "
                      "Unverified devices keep two latest packages on explicit cleanup. "
                      "One unassigned package.",
            "warning": "USB flash success is not boot verification. Legacy devices without "
                       "a recorded OTA-verified build may have an older installed image; "
                       "review before explicit cleanup.",
        }

    def cleanup(self, active_job: str | None = None, manual: bool = False) -> dict:
        _records, _kept, removable = self._plan(active_job, manual=manual)
        deleted = 0
        freed = 0
        for item in removable:
            folder = self.root / item["job_id"]
            info_path = folder / "build_info.json"
            if (folder.is_symlink() or not folder.is_dir()
                    or info_path.is_symlink() or not info_path.is_file()):
                continue
            try:
                meta = json.loads(info_path.read_text("utf-8"))
                if meta.get("job_id") != item["job_id"] or meta.get("device_id") != item["device_id"]:
                    continue
                if any(entry.is_symlink() or not entry.is_file()
                       for entry in folder.iterdir()):
                    continue
                shutil.rmtree(folder)
                deleted += 1
                freed += item["bytes"]
            except (OSError, ValueError, TypeError):
                continue
        return {"removed_count": deleted, "freed_bytes": freed,
                "storage": self.report(active_job=active_job)}

    def mark_ota_verified(self, device_id: str, job_id: str) -> None:
        if not isinstance(device_id, str) or not device_id or not isinstance(job_id, str) or not JOB_RE.fullmatch(job_id):
            raise ValueError("Invalid verified RFLink device/build identity")
        matches = [r for r in self._records()
                   if r["job_id"] == job_id and r["device_id"] == device_id]
        if len(matches) != 1:
            raise ValueError("OTA-verified firmware package is missing or belongs to another device")
        verified = self._confirmed()
        verified[device_id] = job_id
        self._write_confirmed(verified)

    def forget_device(self, device_id: str) -> None:
        verified = self._confirmed()
        if device_id in verified:
            del verified[device_id]
            self._write_confirmed(verified)
