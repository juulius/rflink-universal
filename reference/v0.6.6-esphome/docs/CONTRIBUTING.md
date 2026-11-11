# Contributing to RFLink Universal

Contributions are welcome after the project's license clearance and public launch. Until then, treat this as a draft source snapshot.

Please use an Issue for reproducible bugs or a feature request, explain the board revision and firmware version, and avoid publishing passwords, secret keys, personal RF IDs or private device data. Decoder changes should be isolated in a decoder pack, contain a valid manifest and provide deterministic software tests and clear notes about physical verification. Preserve protocol-source provenance and licenses. Update English, Dutch, French and German translation keys together; never add untranslated hard-coded UI labels.

Review changes against the existing update/merge safety logic. Do not silently change GPIO assignments, transmit defaults, OTA/API secrets, NVS device identity, or any user's original YAML. A code review or host test alone never justifies saying a protocol is hardware-verified.
