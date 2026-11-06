# RFLink Universal Builder (Home Assistant app)

Compile native ESP-IDF firmware for RFLink Universal, compare GitHub source updates, select previous firmware releases, and prepare local USB or OTA installation. The app maintains user-specific board overrides and credentials in Home Assistant's persistent local storage rather than in GitHub.

**Development version:** v0.5.2 — GitHub Actions has successfully compiled the native ESP-IDF firmware, but installation of this Builder app on Home Assistant OS, device flashing and RF operation have not been validated.

**Before installing:** this app builds locally and downloads the official `espressif/idf:v5.5.6` Docker base. The base image is unusually large (approximately 5 GB compressed), so the first install can be slow and may need considerable available disk space. If the installation stays on a spinner, inspect **Settings → System → Logs → Supervisor** for the actual error (download, storage, timeout or Docker build); the app's own log is not available until it has installed.

Follow [the app instructions](DOCS.md) only after this repository has been imported and its installation prerequisites verified.
