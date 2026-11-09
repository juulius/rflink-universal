# Radio Gaga v0.6.6 radio stack → native ESP-IDF v0.7

**Architecture decision (2026-11-09):** Keep RFLink Universal a standalone
native ESP-IDF firmware. The working v0.6.6 ESPHome stack is the
**behavioral/source reference**, not a runtime, YAML or firmware dependency.
The earlier read-only ESPHome hybrid probe (PRs #73/#74) is a separate
experiment and is **not the intended production architecture**.

## Evidence and scope

On the user's unchanged ESP32-S3 board with three CC1101s, flashing
v0.6.6 Radio Gaga restored all three radio-online indications. Native
v0.7 Experimental build 7 repeatedly emitted
`Receiver restore failed` / `ManagerError::driver_failure` (7) on both
433 MHz receivers around five seconds after boot and every ~60 seconds.
This establishes software-regression suspicion, but **not** the exact
failed ESP-IDF/RMT API or actual RF reception on each radio.

The verified v0.6.6 source archive is
`RFLink_Universal_v0.6.6_RadioGaga_RELEASE_HASH_FIX_SOURCE.zip`
(SHA-256 `d91dde4f579a92e5313b13e6499de45ff4758ad149045817f6014b444a519a8f`).
It is **not** committed to this repository: it may include non-essential
user configuration and its full tree is not necessary for this patch.
Source landmarks in that archive:
- `rflink-universal.yaml`: three CC1101 modules, two 433 MHz
  `remote_receiver` and `remote_transmitter`, 868 packet callback,
  setup order, ten-second watchdog and controlled reset/re-init.
- `rflink_universal/components/rflink_universal/radio_boot_guard.cpp`:
  pre-power high-impedance setup, delayed safe SPI ownership, non-destructive
  CHIP_RDYn/SO checks, GDO0 RX/TX function ownership.
- `.../cc1101_packet_rx_restore.h`, `.../cc1101_gdo_uart_tx.h`,
  `.../gdo_serial_tx_session.h`: explicit CC1101 RX restoration and TX
  resource cleanup, to be compared line by line with native transactions.

## Fidelity matrix and implementation gates

| Reference behavior in working v0.6.6 | Native v0.7 work | Status |
| --- | --- | --- |
| All radio GPIOs input/floating for 200 ms after application radio initialization starts | `rflink_native_boot_guard.cpp` | Implemented in native source; host test gate, hardware pending |
| CSn high+pull-up, SCK/MOSI low+pull-down, MISO input+pull-up before SPI | Same boot guard | Implemented; hardware pending |
| One-at-a-time 5 × 10 ms CHIP_RDYn/SO checks, MISO release check, 25 ms quiet gap | Same boot guard; then existing `Cc1101SpiBus` | Implemented as read-only nonfatal probe; hardware pending |
| CC1101 identity, modem-register programming, PLL calibration, GDO0/GDO2 routing and profile ownership | Native SPI, profiles and radio manager, **compare exact register-by-register and status transitions** | Partial; not hardware-equivalent |
| Two 433 OOK RX GDO2 pins 6/14, non-DMA 96-symbol RMT receive channels, pull-down, filter 30 µs, idle 5 ms | Native RMT RX | Config values copied; v0.7 build 7 still fails; parity **not proven** |
| One 868.2999 MHz FSK packet receiver with 63-byte FIFO and GDO0 IRQ | Native 868 packet RX | Partial; actual interrupt/FIFO behavior and RF reception pending |
| Two 433 OOK TX drivers, GDO0 safe switching, RMT allocation plus shared resources | Native RMT TX + radio manager | Partial; need literal allocation and RX→TX→RX parity |
| 868 packet / optional UART transmit, arbitration, serial GPIO restoration | Native packet/UART TX + radio manager | Partial; on-air TX and restore not verified |
| Ten-second watchdog with failed-component reset/re-setup and nonfatal missing radio | Native watchdog & radio recover | Different retry model; need parity & intentional explicit reset only under serialized hardware ownership |
| Persistent diagnosis, RX/TX event logs and radio-specific failure reasons | Native logging + Builder | Present in prototype; not full v0.6 parity or real crash verified |

**Pinout / baseline:** shared SCLK 12, MOSI 11, MISO 13;
433.42 CSn/GDO0/GDO2 4/5/6; 433.92 9/8/14; 868.30 15/16/17.
Default SPI speed 200 kHz. Do not infer that module presence alone
proves GDO/RMT reception. Do not assume the static 433 OOK profile is
register-identical to the working component until readbacks are compared.

## Verified build gate and potential 433 failure before RMT

PR #75 initially failed the official ESP-IDF compile because
`gpio_set_pull_mode()` expects `gpio_pull_mode_t`, not a plain integer.
The native guard now passes that exact enum type; the host stub mirrors the
type and the official compile must pass before merging. In the separate
433 startup path, the high-level `ManagerError::driver_failure` (7) can
occur during SPI `SIDLE`, frequency/modem register programming or readback,
RMT channel setup, **or** verified RX entry. An absent RMT failure stage
therefore does not prove the radio driver succeeded. Follow-on development
must make each pre-RMT failure visible in read-only diagnostics before
changing a live radio profile based on speculation.

## Native port sequencing

1. **Boot guard — this PR.** Before enabling SPI, run the 200 ms
   high-impedance phase, safe bus idle setup, 5 bounded read-only probes per
   CC1101, and 25 ms post-probe guard. GPIO setup failures fail closed.
   A missing radio's `CHIP_RDYn` is nonfatal; the native SPI identity
   probe subsequently determines actual presence. Preserve log visibility
   without claiming that a successful probe is RX.
2. **CC1101 setup equivalence.** Use the exact ESPHome component's reset,
   modem register and calibration order, CSn/SO contract and async GDO
   routing. Record the **first** failure (register, readback, MARCSTATE,
   RMT stage, underlying `esp_err_t`). Test all three states without
   writing to another radio's CSn.
3. **RMT parity.** Trace two ESPHome `remote_receiver` initialization
   sequences (including GPIO matrix/pull ownership, channel reservation,
   `rmt_receive` buffer and ISR rearm) against native ESP-IDF.
   Explicitly account for two 96-symbol non-DMA RX allocations, separate
   48-symbol non-DMA 433 TX allocations and the LED TX reservation.
   Do not assume a register-parity patch fixes RMT channel allocation.
4. **RX/TX arbitration and reinitialization.** Port the complete
   v0.6.6 serial/GDO session lifecycle and failure-driven component
   setup/reset path under one native per-radio owner. Transmitting must
   not prevent other CC1101 receivers from operating. No uncontrolled
   `SRES` while another task owns a radio.
5. **Integration and benchmark.** Preserve v0.7 native decoder/manifest
   architecture, Builder/OTA and logging; validate device pairing and
   universal TX only after reception is proven.

## Acceptance on hardware (separate bench device)

- All three CC1101s present **and listening** at cold and warm boot.
- Correct 433.42 and 433.92 pulse reception with distinct sources, plus
  868 packet reception; run at least two watchdog periods with no
  recurrent restore failure.
- 433 RX→TX→RX and 868 RX→TX→RX recover with actual receiver events;
  no reboot and no inadvertent TX. A successful driver return is **not**
  proof of on-air transmission.
- Unplug/missing radio cannot block other radio's SPI or Wi-Fi;
  reconnection and radio reset are deliberate and observable.
- Confirm Web/OTA before radio setup, capture reset history/early panic,
  and verify OTA rollback or downgrade safely using a dedicated board.
- Measure memory, boot duration, receiver counters and packet loss against
  the same physical v0.6.6 reference.

The **full native port is not complete** in this first PR. Do not mark
v0.7 stable, publish a test build as hardware-validated, or overwrite the
known-working v0.6.6 installation based only on successful CI.
