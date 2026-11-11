# RFLink Universal — radio profiles for user protocols

Status: **USERLIB2**. This file separates a *protocol recipe* from the physical CC1101 configuration.

## Current validated profiles

| Profile | Radios | Frame type | Current TX contract |
|---|---|---|---|
| `board_raw` | 0 and 1 | signed RAW pulses | max. 512 pulses; 50–1,000,000 µs per pulse |
| `board_packet_63` | 2 | packet | exactly 63 bytes with the shipped 868 MHz CC1101 setup |

The `/rflink/api/user-library` response exposes these as `radio_profiles` and every user decoder reports its active `radio_profile`.

## Why USERLIB2 does not expose arbitrary CC1101 registers

A decoder recipe describes how a received frame is interpreted and how a transmit frame is constructed. Modulation, frequency, data rate, deviation, sync word and packet mode determine whether the CC1101 can produce or receive that frame at all. Allowing arbitrary register writes from a user recipe would bypass the board's existing radio safety and could silently break Itho/RAW reception.

Therefore USERLIB2 deliberately supports only the board-validated profiles above. A recipe can be a completely new **protocol within that radio profile**, but it is not yet a completely new **radio PHY**.

## Planned safe extension

Future radio support should use named, reviewed presets rather than free-form register values:

1. a preset has a stable ID and explicit band/modulation/bitrate/packet contract;
2. RX and TX capability are declared separately;
3. a preset is host-tested and then hardware-verified before normal users can select it;
4. changing a decoder's preset is treated transactionally and replays its saved captures;
5. firmware promotion records the required preset and refuses a missing/unsupported one;
6. Advanced may show raw CC1101 registers for diagnostics, but importing a recipe never grants arbitrary register writes.

This keeps the route open for additional 868/433 profiles without turning a user decoder into unrestricted radio-driver code.
