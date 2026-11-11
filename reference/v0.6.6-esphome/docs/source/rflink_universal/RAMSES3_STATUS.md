# RAMSES3 Radio Safety Foundation

This is a **pre-hardware** stage of multi-protocol 868.30-MHz support for RFLink Universal.

The Itho packet receiver stays active. The new `RadioModeManager` is compiled into the main RFLink component, but its physical hardware backend is deliberately unarmed. No live Orcon pairing or transmit controls are enabled by this update.

Scope: exclusive radio lease, non-blocking transitions, max 30-second lease, fail-closed restoration, deferred watchdog SPI probes and blocking of packet transmission before NVS updates, with host tests.

See `RAMSES3_RADIO_SAFETY_NL.md` for physical-backend requirements and the commissioning sequence.

Validation: host state-machine tests, existing RAMSES-II RX/TX codec tests, RFLink core-monitor test, syntax check, decoder catalogue tests. **Not** yet ESPHome/ESP-IDF compiled nor physically tested.


RAMSES4 addendum: a SPI-only CC1101 lab backend can now be armed explicitly
using `ramses_spi_lab: true` and a manual RX lease request. The default remains
`false`; TX and physical pairing are still not enabled. See RAMSES4_SPI_LAB_NL.md.
