# RFLink Universal Builder — Home Assistant

> **Development app:** A full Home Assistant OS installation and RF hardware flash/OTA test have not yet been verified. Do not replace a working v0.6.6 Radio Gaga gateway.

## Install

1. Open **Settings → Apps → Install app → ⋮ → Repositories** in Home Assistant OS.
2. Add `https://github.com/juulius/rflink-universal` once and refresh the app store.
3. Choose **RFLink Universal Builder** and select **Install**.
4. Start the app and open its web interface. You should see the **Build** page immediately, even while the compiler downloads.
5. On **Build → Compiler setup**, follow the live setup console. The official **ESP-IDF v5.5.6** source, ESP32-S3-specific tools and Python packages are downloaded and installed into persistent `/data/toolchain` storage. This can still need considerable space, time and internet access.
6. When Compiler setup shows **Ready**, check your local source and hardware profile, then choose **Build firmware**. Do not flash until you have a verified test-board plan.
7. Use **Versions & GitHub** for source comparisons and release selection; **Settings** holds local GPIO, secrets and MQTT settings.

The official source repository `juulius/rflink-universal` is configured automatically. It is not necessary to save or re-enter it. If you intentionally use another public repo, expand **Use another repository (advanced)** in **Versions & GitHub**.

## Progress, errors and retries

- **App installation is still spinning:** inspect **Settings → System → Logs → Supervisor**. The app has not started yet, so its own setup console cannot report Docker installation errors.
- **App opens, compiler is downloading:** open **Build → Compiler setup → Live setup console**. The installer runs inside the started app rather than as part of the Home Assistant app image installation.
- **Compiler setup fails:** read the full console; use **Retry setup** once the network/storage issue is corrected. The toolchain never reports ready until the ESP-IDF version check passes.
- **Compiler is ready but Build fails:** open **Build console** for the individual compiler errors. Success here is not proof of RF or OTA functionality.
- **Restart/reinstall:** the toolchain cache is in the persistent `/data/toolchain` directory. The app uses a version marker and should reuse a successful matching installation. Restore only your local settings and build files from backups; downloading compiler files again is preferable to backing up gigabytes.

## Firmware releases

The default GitHub Release selector shows the previous five published stable firmware releases. The **All releases** view includes older and prerelease versions. Selected tags resolve to immutable commit identities for isolated builds using your local board overrides. Firmware versions and the Home Assistant Builder version are separate.

## Local storage and safety

Personal GPIO settings, SPI clock, Wi-Fi/OTA credentials, MQTT credentials and build artifacts stay in the local Home Assistant app storage; do **not** commit them to public GitHub. These credentials are not automatically embedded in a firmware image.

Current OTA/mqtt provisioning experiments are over the trusted LAN and are **not** safe for direct internet exposure. Use a separate ESP32-S3 N16R8 test board for initial installation. Native v0.7 OTA is not a supported direct upgrade from v0.6.6.

The current Builder pipeline is a development prototype; full app startup, toolchain downloads on amd64/aarch64 Home Assistant hosts, and initial USB/OTA installations require practical verification.
