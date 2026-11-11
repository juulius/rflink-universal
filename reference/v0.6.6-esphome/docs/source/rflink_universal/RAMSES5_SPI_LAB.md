# RAMSES5 SPI-only receive diagnostics (experimental)

This optional RFLink Universal lab update adds a second CC1101 RX-FIFO capture mode that does not depend on the experimental B32A hardware-sync match. It performs software RAMSES-II UART/Manchester framing and checksum validation, and leaves transmission and pairing disabled. See `RAMSES5_SPI_LAB_NL.md` for the complete hands-on checklist and diagnostics.

- `ramses_spi_lab: false` by default. Only an explicit, bounded RX test can acquire the shared 868.30 MHz radio.
- `ramses_spi_unsynced: false` uses B32A hardware sync; `true` selects an unframed RX-FIFO software-sync experiment. This is **not a proven replacement** for GDO/UART transparent streaming.
- Explicit 26 MHz CC1101 modem parameters: 38.4 kbps 2-FSK, ±50.78 kHz deviation, 325 kHz RX bandwidth. The original Itho configuration is backed up, restored, and verified before the radio becomes available again.
- RX FIFO race workaround, cumulative per-radio diagnostics, fail-closed restore, and host regression tests for checksum, FIFO underflow/overflow, status reads, radio leases, and decoder precedence.
- Hardware RF interoperability, full ESPHome compile, persistent live RX and physical TX/pairing **are not verified**. Do not enable TX based solely on host tests.
