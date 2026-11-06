# Public publication checklist

This is a work-in-progress public repository. Before importing the full v0.6.6 source and current native firmware:

- [ ] Review each bundled third-party decoder, ported library and copied source for provenance and license obligations.
- [ ] Agree on project software/documentation/hardware licensing; include the complete applicable notices.
- [ ] Remove actual passwords, tokens, signing keys, user-paired device identifiers, saved MQTT settings and local network configuration.
- [ ] Keep only **default/example** board pinouts and SPI speed in GitHub; retain user overrides in Home Assistant's persistent local data.
- [ ] Remove generated build output, environment caches, personal file metadata and diagnostic captures unless expressly cleared.
- [ ] Make the distinction between untested v0.7 native firmware and working v0.6.6 reference unambiguous.
- [ ] Confirm Home Assistant app installation and a complete official ESP-IDF build before publishing firmware binaries as ready to flash.
- [ ] Require board/profile identity checks for OTA and USB installs; test downgrade and rollback behavior.
- [ ] Publish firmware GitHub Releases only from tagged, reproducible and validated source.

Reference: publication and licensing notes in the existing v0.6.6 Radio Gaga archive. No project-wide license is implied by public visibility.
