# RFLink v0.7 public publication checklist

- [ ] Audit third-party and migrated protocol code for attribution and distribution rights.
- [ ] Choose the project license and provide all required notices.
- [ ] Keep passwords, tokens, WiFi credentials, MQTT accounts, private keys, paired-device IDs and local network settings out of GitHub.
- [ ] Publish only the **default/example** board pinout and SPI speed; preserve personal overrides inside Home Assistant Builder persistent local storage.
- [ ] Include only documentation that matches the implemented v0.7 code. Do not bulk-import obsolete v0.6 material.
- [ ] Exclude generated build output, private diagnostics and personal captures.
- [ ] Verify a full official ESP-IDF build and check flash/PSRAM/partition settings for ESP32-S3 N16R8.
- [ ] Test CC1101 detection, all RX paths, TX/RX recovery, local OTA and rollback on an actual device.
- [ ] Confirm the Home Assistant app installs and upgrades without overwriting local settings.
- [ ] Release firmware binaries only from tagged builds whose board identity, source commit and test status are recorded.

This repository is not yet ready for a stable firmware release.
