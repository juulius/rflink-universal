# Experimental 868 MHz GDO DATA+CLOCK sampler — review and bench gates

**State: native adapter source, not a live RF receiver.** This PR adds an
ESP-IDF GPIO interrupt sampler and a bounded interrupt-to-task transfer.
It is compiled into the HAL but **never constructed or started by main.cpp**.
The running RFLink still uses its existing fixed 63-byte 868 MHz SPI/FIFO
receiver. No web control, Builder toggle, OTA setting, GPIO mode switch,
radio register write, TX/pairing action, or new live decoder is enabled.

## Intended everyday result (not available yet)

On the third CC1101, one permanent 868.30 MHz 2-FSK modem configuration
would supply synchronous received DATA and CLOCK. Itho and RAMSES-II
software decoders would examine the same continuous bitstream without
periodically changing the CC1101 from FIFO to UART.

The physical GDO sampler must *not* be enabled for regular operation until
its interrupt cost, missing-bit behavior, RF compatibility and TX restore
have been tested on the user's actual board.

## What is implemented

- `SyncClockEdgeRing`: single-producer GPIO ISR to single-consumer RX worker,
  2,048 events in bounded internal memory; no allocation, SPI, logging,
  decoder calls or FreeRTOS queue operations in the ISR.
- Drop **newest** on full ring. Every lost edge is counted. The next accepted
  sample carries `lost_before`; it causes the existing
  `ClockedBitCapture::report_lost_edges` to abandon the incomplete capture
  before recording new bits. When ISR shutdown leaves only trailing drops,
  the stop path reports them explicitly.
- `Cc1101SyncGdoSampler`: checks that the caller requests only radio 2,
  uses the compiled board profile's GDO0 (DATA) and GDO2 (CLOCK) as ESP32
  inputs, and rejects an object outside internal RAM. The GPIO ISR samples
  the data GPIO register at the selected clock edge and reads a timestamp.
- The GPIO ISR service is installed with `ESP_INTR_FLAG_IRAM`. If another
  service already owns it, this bench adapter **fails closed**, since that
  service's cache-disabled/IRAM behavior cannot be verified. Once installed,
  it is intentionally not uninstalled because it is a globally shared
  ESP-IDF service. This policy must be reviewed before normal integration.
- `start`, `poll_once` and `stop` are **not thread-safe with one another**
  and must be serialized by the future RadioManager owner. Shutdown stops
  interrupts, removes the callback, drains accepted bits in order and
  publishes an explicit acquisition-window boundary.
- Deterministic host tests cover full-ring loss, precise gap markers,
  recovery and concurrent single-producer/single-consumer ordering.
  These synthetic tests do not reproduce real interrupt latency, clock-edge
  race conditions, or ESP32 GPIO-matrix behavior. Queue loss counts describe
  software overflow only; they cannot detect an interrupt that was never
  dispatched by the hardware. The
  official ESP-IDF build compiles the real GPIO driver source but does not
  execute it on real RF hardware.

There is **no physical CC1101 register configuration in this PR**, and the
sampler cannot prove DATA/CLOCK timing from its own existence. A normal
firmware update does not activate it.

## Pins, RF profile, and critical safety issue

| Third CC1101 line | Compiled reference ESP32 GPIO | Future purpose |
|---|---:|---|
| GDO0 | GPIO16 | synchronous demodulated DATA |
| GDO2 | GPIO17 | synchronous recovered CLOCK |
| CSN / SPI bus | GPIO15 / shared 12,11,13 | CC1101 register and health management |

Use the **actual Builder-generated profile**: reference pins may have been
overridden locally. Never hardcode GPIO16/17 into the runtime driver.

TI CC1101 Table 41 describes `IOCFG0=0x0C` for synchronous DATA and
`IOCFG2=0x0B` for CLOCK (candidate non-inverted outputs), with
`PKTCTRL0.PKT_FORMAT=01` for synchronous serial mode. For non-inverted RX,
the CC1101 sets up DATA on the falling CLOCK edge; sampling on the rising
edge is a **candidate**, not a tested fact on this board.

**GDO0 is also our existing 868-MHz UART TX path.** It must not be
configured as a GPIO input/sampler while UART TX is driving it. Only
RadioManager, with its exclusive RX/TX lease, may start/stop the future
physical adapter. The current bench code is purposely not wired to TX.
Any future physical start must first stop the FIFO receiver, enter safe
IDLE, apply a complete and verified synchronous modem profile and then
start the GPIO adapter; TX must stop the adapter and restore the **same
synchronous RX**, not silently switch between FIFO and UART.

## Why this is only a proof-of-feasibility ISR path

At approximately 38.4 ksymbols/s the ESP32 could service around **38,400
GPIO interrupts per second** continuously. Even though the ISR only reads
a GPIO register, samples a microsecond timer and writes one bounded slot,
the result is not yet tested against cache-disabled periods, flash writes,
Wi-Fi, two 433-MHz RMT receivers, watchdogs or task scheduling.

The 2,048-event ring holds only approximately **53 ms** at 38.4 ksymbols/s
before backpressure starts. If the CPU cannot consistently drain the ring,
the correct engineering response is to switch the **capture peripheral**
to a verified DMA- or hardware-clocked mechanism, not switch the CC1101
between FIFO and UART receive modes.

Do not assume ESP32-S3 SPI-slave works as a free-running serial sniffer:
ESP-IDF describes SPI-slave transfers as chip-select-framed transactions.
The ESP32-S3 LCD_CAM camera port can sample a parallel bus with an external
pixel clock and DMA, but its VSYNC/HREF gating and continuously usable
one-bit capture need hardware-specific investigation before use. Neither
path has been implemented in this PR.

## Required next PR and physical acceptance

1. Choose and implement the actual **manager-owned** start/stop/profile
   lifecycle, behind a build-time Experimental gate, default disabled.
   Require readback of all changed CC1101 modem/GDO registers and safe
   rollback on any failure. Do not expose it through ordinary web actions.
2. Bench the clock+data signal with a scope/logic analyzer. Confirm the
   expected edge, 2-FSK bit rate, pin voltage and polarity before accepting
   any captured bitstream as protocol evidence.
3. Measure interrupt count, ring fill/drop, task priority and jitter while
   433.42 and 433.92 receive, Wi-Fi reconnects, OTA loads and logs stream.
   Include forced interrupts, missing edges and corrupted clocks.
4. Prove **real Itho AND RAMSES-II traffic with one fixed 868 RF profile**.
   If their RF settings are physically incompatible, do not automatically
   hop transports; consider a dedicated extra CC1101 or revise scope.
5. Add a bounded bitstream/frame reassembler for both protocols and validate
   exact end-of-frame, checksum/integrity, ambiguity and missing-bit handling.
6. Verify TX ownership and **TX->same-synchronous-RX** restore on the real
   ESP32. Retain the old FIFO firmware as a separate A/B rollback image,
   not a runtime fallback.

## References

- [TI CC1101 data sheet, synchronous serial GDO signals](https://www.ti.com/lit/ds/symlink/cc1101.pdf)
- [ESP-IDF 5.5 GPIO interrupt service, ESP32-S3](https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32s3/api-reference/peripherals/gpio.html)
- [ESP-IDF SPI slave, ESP32-S3](https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32s3/api-reference/peripherals/spi_slave.html)
- [Current fixed-RX architecture](CC1101_UNIVERSAL_RX_ARCHITECTURE.md)
- [Host-only clocked bit capture](CC1101_868_FIXED_CLOCKED_RX.md)
