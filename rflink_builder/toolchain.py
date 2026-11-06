"""Persistent on-demand ESP-IDF setup for a lightweight Home Assistant Builder.

The API remains usable while the official toolchain installs. Only fixed,
version-pinned Espressif commands are executed; never user-supplied shell.
"""
from __future__ import annotations

import os
import shutil
import subprocess
import threading
from pathlib import Path

IDF_VERSION = "v5.5.6"
TARGET = "esp32s3"


class ToolchainManager:
    def __init__(self, data: Path):
        self.root = Path(data) / "toolchain"
        self.idf = self.root / ("esp-idf-" + IDF_VERSION)
        self.tools = self.root / "tools"
        self.marker = self.root / "installed-version"
        self.log_path = Path(data) / "toolchain-setup.log"
        self.lock = threading.RLock()
        self.status = "ready" if self.is_ready() else "idle"
        self.error = ""

    def is_ready(self):
        try:
            return (self.marker.read_text(encoding="utf-8").strip() == IDF_VERSION
                    and (self.idf / "export.sh").is_file()
                    and (self.idf / "tools" / "idf.py").is_file())
        except OSError:
            return False

    def snapshot(self):
        with self.lock:
            return {"status": self.status, "version": IDF_VERSION,
                    "target": TARGET, "error": self.error,
                    "ready": self.is_ready()}

    def read_log(self, offset: int = 0):
        try:
            size = self.log_path.stat().st_size
            offset = max(0, min(int(offset), size))
            with self.log_path.open("rb") as stream:
                stream.seek(offset)
                raw = stream.read(48 * 1024)
                next_offset = stream.tell()
            return {"text": raw.decode("utf-8", errors="replace"),
                    "next_offset": next_offset, "size": size}
        except (OSError, ValueError):
            return {"text": "", "next_offset": 0, "size": 0}

    def start(self):
        with self.lock:
            if self.status == "running":
                return self.snapshot()
            if self.is_ready():
                self.status = "ready"
                self.error = ""
                return self.snapshot()
            self.status = "running"
            self.error = ""
            self.root.mkdir(parents=True, exist_ok=True)
            threading.Thread(target=self._install, daemon=True, name="rflink-toolchain-setup").start()
            return self.snapshot()

    def _command(self, argv, env, log, cwd=None, timeout=3000):
        log.write("$ " + " ".join(argv) + "\n")
        log.flush()
        completed = subprocess.run(argv, cwd=cwd, env=env, stdout=log,
                                   stderr=subprocess.STDOUT, timeout=timeout, check=False)
        if completed.returncode:
            raise RuntimeError(f"Setup command failed (exit {completed.returncode}): {argv[0]}")

    def _install(self):
        env = dict(os.environ)
        env.update({"IDF_TOOLS_PATH": str(self.tools), "IDF_PATH": str(self.idf),
                    "PIP_DISABLE_PIP_VERSION_CHECK": "1",
                    "PYTHONUNBUFFERED": "1", "GIT_TERMINAL_PROMPT": "0"})
        try:
            self.root.mkdir(parents=True, exist_ok=True)
            # A failed or interrupted install must never leave a ready marker.
            self.marker.unlink(missing_ok=True)
            with self.log_path.open("w", encoding="utf-8", buffering=1) as log:
                log.write(f"RFLink Builder: preparing official ESP-IDF {IDF_VERSION} for {TARGET}.\n")
                if not (self.idf / "install.sh").is_file():
                    if self.idf.exists():
                        shutil.rmtree(self.idf)
                    self._command(["git", "clone", "--depth", "1", "--branch", IDF_VERSION,
                                   "--recurse-submodules", "--shallow-submodules",
                                   "https://github.com/espressif/esp-idf.git", str(self.idf)],
                                  env, log, timeout=2700)
                else:
                    # Repair submodule checkout after an interrupted setup.
                    self._command(["git", "submodule", "update", "--init", "--recursive",
                                   "--depth", "1"], env, log, cwd=self.idf, timeout=2700)
                self._command(["bash", "install.sh", TARGET], env, log,
                              cwd=self.idf, timeout=2700)
                activation = self.environment()
                result = subprocess.run(["idf.py", "--version"], env=activation,
                                        cwd=self.idf, capture_output=True, text=True, timeout=60)
                log.write(result.stdout + result.stderr)
                if result.returncode or "5.5.6" not in result.stdout:
                    raise RuntimeError("Toolchain version check failed; expected ESP-IDF 5.5.6.")
                temporary = self.marker.with_suffix(".tmp")
                temporary.write_text(IDF_VERSION + "\n", encoding="utf-8")
                temporary.replace(self.marker)
                log.write("\nESP-IDF is installed and ready to build.\n")
            with self.lock:
                self.status = "ready"
        except Exception as exc:
            with self.lock:
                self.status = "failed"
                self.error = str(exc)[:350]
            try:
                with self.log_path.open("a", encoding="utf-8") as log:
                    log.write("\nERROR: " + self.error + "\n")
            except OSError:
                pass

    def environment(self):
        """Return the environment produced by the official export.sh script."""
        env = dict(os.environ)
        env.update({"IDF_PATH": str(self.idf), "IDF_TOOLS_PATH": str(self.tools)})
        if not (self.idf / "export.sh").is_file():
            raise RuntimeError("ESP-IDF is not downloaded yet")
        command = 'set -e; . "$IDF_PATH/export.sh" >/dev/null 2>&1; env -0'
        result = subprocess.run(["bash", "-c", command], env=env,
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                timeout=90, check=False)
        if result.returncode:
            raise RuntimeError("ESP-IDF environment activation failed; see setup log.")
        values = {}
        for pair in result.stdout.split(b"\0"):
            if b"=" not in pair:
                continue
            k, v = pair.split(b"=", 1)
            values[os.fsdecode(k)] = os.fsdecode(v)
        if "PATH" not in values:
            raise RuntimeError("ESP-IDF export returned no PATH")
        return values
