# RFLink Universal Builder — Home Assistant

> **Development app:** A full Home Assistant OS installation and RF hardware flash/OTA test have not yet been verified. Do not replace a working v0.6.6 Radio Gaga gateway.

## Install

1. Open **Settings → Apps → Install app → ⋮ → Repositories** in Home Assistant OS.
2. Add `https://github.com/juulius/rflink-universal` once and refresh the app store.
3. Choose **RFLink Universal Builder** and select **Install**.
4. Start the app and open its web interface. You should see the **Devices** dashboard immediately, even while the compiler downloads.
5. Select **Add device → New device**. Enter a name, review your copied global Wi-Fi and pinout defaults, optionally change this device's hardware settings, and accept or regenerate its unique OTA password. Choose the source version and select **Create device**.
6. Select **Build firmware** for the saved RFLink; its board configuration is isolated from other RFLinks and the global defaults.
5. Select a device and use **Firmware → Build firmware** to reach the compiler setup page, then follow the live setup console. New test boards can also start a build through **Add device**. The official **ESP-IDF v5.5.6** source, ESP32-S3-specific tools and Python packages are downloaded and installed into persistent `/data/toolchain` storage. This can still need considerable space, time and internet access.
8. When Compiler setup shows **Ready**, check your local source and hardware profile, then choose **Build firmware**. Do not flash until you have a verified test-board plan.
9. The **Devices** dashboard discovers native RFLinks; use **Add device** to adopt a gateway, then open it. **Overview** holds name and its own OTA key (with an eye to reveal it), **Connection** shows IP and status, **Firmware** links to build/release/OTA, and **Advanced** holds infrequent actions. Shared advanced Builder settings are behind the gear icon. Source/GitHub history and the compiler remain separate tools, accessed from each device rather than displayed on the dashboard.

**Device discovery limitation:** Unlike ESPHome's modern Device Builder, current RFLink firmware does not announce a custom mDNS service. For now the Builder scans only private /24 LAN ranges (defaults: 192.168.178.0/24 and 192.168.1.0/24) and supports explicit IP entry for VLANs. Already managed devices are checked periodically and stay visible when offline. On legacy native firmware without a stable hardware device ID, saved passwords are tied to an IP address; a DHCP reservation is recommended to avoid address reuse. A future firmware change will introduce real identity-aware discovery.

The dashboard's **New published release** indicator compares release versions only; it does **not** mean a compatible OTA image has been built, validated, or flashed. The OTA action remains behind the existing board/profile and TESTBOARD checks. The **Prepare OTA install** button only becomes enabled when a verified local firmware build matches the current RFLink's board ID and profile hash, it is online, and it has a saved OTA key. The RFLink must still be a test board. Board GPIO/SPI settings in this Builder are currently **shared**, not per-device build profiles; the device details screen makes this limitation explicit.

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

## Initial installation with per-device settings

The device build generates a sensitive provisioning NVS image for the native firmware's `rf_net` namespace (`ssid`, `pass`, and `update_key`) at partition **0x9000**, size **0x10000**. The first installation via **USB connected to the Home Assistant host** writes bootloader, partition table, OTA metadata, application and the device-specific NVS image. The Builder checks that the selected device and its configuration revision match the build before installation, and retains the TESTBOARD confirmation.

You may instead **download the private USB ZIP** and flash the included components from another computer using ESP-IDF/esptool and the offsets in its release manifest. This ZIP contains plaintext Wi-Fi and OTA secrets via the NVS image. Do not store it in cloud shares, GitHub, or publicly accessible folders. The current firmware does not support hashed OTA-key storage or encrypted OTA transport; generating a high-entropy key is useful but does **not** fix those limitations.

After the first USB installation, the RFLink should appear under **Found on your network** if reachable. Open the saved RFLink's Advanced panel and explicitly link the discovered device to its saved configuration. Device identity and board-profile hashes are checked. Later OTA requires a connected RFLink, saved key and compatible build; there is no wireless first install for an unflashed board. A full real Home Assistant OS install/flash remains unverified.
