# RFLink Universal web UI — v0.3.2-RC3

RFLink serves a dependency-free mobile-first interface at:

`http://<device-ip>/` (redirects to `/rflink`)

## Normal-user principle

The normal interface avoids terms such as GDO, CRC, Manchester or pulse arrays unless the user deliberately opens Developer/diagnostic information.

### Status

Shows the radio channels and a **Wat wil je doen?** section:

- Apparaat toevoegen;
- Pairing;
- Onbekend protocol;
- Probleem oplossen.

### Apparaten

Shows confirmed devices and candidates. New RF identities are never merged automatically. The user chooses:

- **Nieuw apparaat**; or
- **Koppel aan bestaand** when RFLink shows a similarity suggestion.

A Pairing filter shows only devices whose decoder declares pairing support. Actual protocol-specific TX controls remain gated until hardware validation.

### Leren

Two clearly separated modes:

- **Apparaat leren** — normal guided workflow;
- **Decoder maken** — Developer Mode with code assistance.

Both use the same safe sequence: radio → baseline → controlled action → repeated candidate.

### Instellingen & hulp

Shows:

- clear task routes;
- decoder engines currently registered in firmware;
- the `RFFrame -> DecodeResult` maintenance rule;
- diagnosis counters and RF terminology behind details.

### Monitor (RADIOMON)

Een read-only pagina met drie radio's plus totaaloverzicht. Afwezige radio's
zijn uitgeschakeld. RX/TX-filters tonen de laatste 32 gebeurtenissen uit de
beperkte geschiedenis in RAM (geen permanente capture, geen WebSocket).
De frequentie, protocol/actie en beschikbare diagnostiek verschijnen per
gebeurtenis. 433 raw: hash en pulsaantal, 868 packet: eerste 16 bytes en RSSI.
TX is uitsluitend een geregistreerde opdracht aan de driver, geen bevestiging
van een werkelijk radiosignaal. 868 MHz werkt via CC1101 SPI-pakketmodus.

### Radioselectie bij Leren

De knoppen worden via de echte `/rflink/api/status`-status bijgewerkt.
Bij geen werkende ontvanger zijn achtergrondmeting en luisteren uitgeschakeld.
Een radio met een defecte RMT-ontvanger wordt niet als leerbaar weergegeven,
ook als de CC1101 zelf gevonden is.

## Lightweight behavior

- no frontend framework/CDN/external font;
- no long-term history or charts;
- no WebSocket/SSE log stream;
- status polling every 3 seconds;
- devices/developer payloads only when relevant;
- UI stored directly in firmware flash.

## REST endpoints

Read-only GET:

- `/rflink/api/status`
- `/rflink/api/devices`
- `/rflink/api/developer`
- `/rflink/api/decoders`

State-changing POST:

- `/rflink/api/developer?action=baseline&channel=2`
- `/rflink/api/developer?action=finish_baseline`
- `/rflink/api/developer?action=listen&channel=2&label=device`
- `/rflink/api/developer?action=reset`
- `/rflink/api/candidate?action=confirm&identity=...&name=...`
- `/rflink/api/candidate?action=link&identity=...&uid=dev_00001`

Developer JSON includes the best candidate's frame kind, repeat count, protocol/identity annotation, RSSI when available and a generated starter snippet.

The API is intended for the local trusted LAN in RC3. Authentication/authorization is a later hardening step.

## RXLOG1 reception history (2026-10-25)

- Filter Monitor by **Signal / Noise / All** and by radio, including the shared noise pool.
- See **Settings → Logging** to change each signal limit and the total noise limit while running.
- Use **Download all logs (ZIP)** to export 8 files: radio-specific signal/noise CSV, merged chronological CSV and diagnostics JSON.
- Browse the paginated `GET /rflink/api/rx-log` endpoint. `GET/POST /rflink/api/rx-log-config` reads and saves limits.
- Logs are retained in bounded volatile PSRAM and cleared on reboot; only log limits survive via NVS.
