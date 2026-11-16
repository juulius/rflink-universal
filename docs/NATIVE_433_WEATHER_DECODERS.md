# Native 433.92 MHz weather decoders — v0.6.6 parity increment 1

## Why these two first

Native v0.7 already has RX transport and bounded user-decoder VM, but its
built-in protocol decoders initially covered only RAMSES-II/Itho on 868 MHz.
To validate the 433.92 MHz end-to-end receive path without mistaking
background RF noise for real commands, this increment ports **two
source-verified, host-tested, receive-only weather decoder families** from
the archived v0.6.6 ESPHome reference:

| Decoder | Band | Signed RAW durations | Integrity | User action |
| --- | --- | --- | --- | --- |
| Acurite 609TXC / 609A1TX | 433.92 MHz | 40 × mark+gap (80 durations) | 8-bit additive checksum | passive measurement |
| Fine Offset WH2 (not WH2A/WH5) | 433.92 MHz | 8-bit 0xFF preamble + 40 data bits (96 durations) | CRC-8 poly 0x31, init 0 | passive measurement |

Ports retain each reference decoder's long/short PWM mark durations,
1000 µs negative gap tolerance, signed polarity, frame-length requirements,
sensor sanity bounds and payload check. The RX worker does not allocate a
new buffer, start an additional thread or alter RMT/CC1101 configuration.
Built-in match evaluation is skipped if a user recipe matches or is ambiguous.

### Verification boundaries

- **Validated in host CI:** complete synthetic signed-pulse captures,
  expected id, temperature, humidity and battery flags from the codec;
  correct event kind and counter from the real native decoder; altered
  checksum/CRC, truncated pulses, wrong band and unrelated noise rejected.
- **Not yet validated:** live 433.92 MHz capture of a physical Acurite or WH2,
  Home Assistant entities, reception reliability, correct behavior for all
  sensor revisions, 433.42 MHz weather reception.
- Full signed RF samples continue in the same existing bounded RAW-detail
  ring. The web log records protocol family only, no sensor address,
  temperature, humidity, raw bits or rolling-counter metadata.
- Counters can be viewed through the read-only status API as
  `weather433_rx.acurite609txc` and `weather433_rx.fineoffset_wh2`.
- **No TX support or pairing** is granted by detecting these sensor families.

## How to validate on hardware

1. Flash an experimental native firmware build containing this PR only
   after GitHub host tests and the full ESP-IDF build are green.
2. Confirm all three radios are online and the old v0.6.6 fallback is
   available; leave the CC1101 register profiles unchanged.
3. Observe normal background for a short baseline and record the two
   `weather433_rx` counters. They should not rise on arbitrary short
   noise frames; this does not imply that the 433 radio is proven reliable.
4. If an actual 433.92 MHz WH2 or Acurite sensor is available, trigger a
   permitted measurement/reset and confirm the right family's event plus
   a corresponding RAW capture in the 433.92 log. If neither sensor is
   available, the synthetic CI test remains valid but no physical RX claim.
5. Separately test an actual known 433 control by pressing it five times,
   with a few seconds between presses. RF Replay should record multiple
   comparable frames; this check is independent of weather decoder support.
6. For the 868 MHz radio, use a compatible known-good device/capture and
   compare the native firmware with the working v0.6.6 configuration.
   Passive 868 noise without the RAMSES/Itho prefix is not a protocol failure.

## Migration follow-on

Next review gates: migrate the remaining v0.6.6 decoder manifests one
protocol at a time, including documented examples and strict negative
tests; ensure official decoders remain separate from optional device
profiles. Only after validated decoder confidence should the web UI offer
an intuitive **Devices → Add device / Create decoder** wizard with
user-labelled actions, repeated captures and explicit RX enablement.
A device or TX approval must never be created automatically.
