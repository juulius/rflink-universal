# Native CC1101 RX recovery — Stage 21 (bench)

## Scope

This is a prototype on ESP32-S3 N16R8 with three CC1101 modules at 433.42, 433.92 and 868.30 MHz. These changes **are not validated on physical hardware** and should **never replace a working Radio Gaga v0.6.6 installation**. The USB/OTA gate still requires a dedicated test board.

The RX worker now starts whenever the shared SPI bus initializes, **even if all three radio RX starts fail**. Every ~5 seconds the worker checks each managed receiver:

- **receiving and listening**: nothing changes. SPI and RX are left alone.
- **transmitting or switching**: never probe, stop or reset the module. No RX watchdog interference with the experimental universal TX driver.
- **receiving but not listening**: ask the RadioManager to stop the old RX transport, then restart the original radio-specific RX profile.
- **idle but healthy**: start the RX profile.
- **fault/unavailable**: read-only PARTNUM/VERSION probe over the shared, CS-protected bus, without SRES. A plausible module is re-enrolled and its matching RX profile started. An absent module is left untouched.

On the 868 MHz packet receiver, two consecutive successful MARCSTATE=IDLE observations trigger a bounded SIDLE/SFRX/SRX re-arm; FIFO overflow recovery remains in place. Three consecutive SPI status read failures mark the listener inactive so the central watchdog can perform the full receive restart; a single transient error does not disable RX. On register or FIFO recovery failure, `listening=false` prevents silently treating RX as healthy.

## Diagnostics

Native `GET /api/status` reports **current** managed radio presence (not only the boot snapshot), operation and listening state. `rx868_recoveries`, `rx868_idle_rearms`, `rx868_spi_errors` and per-radio `spi_ready_timeouts` help diagnose recovery versus hardware faults. The serial log tag `rflink.rxwatch` gives bounded retry and success messages without exposing passwords. A successful restart proves only a completed software transition, **not over-the-air RF reception**.

## Hardware validation checklist

1. Flash to a **spare** N16R8 test board only, record baseline boot log and `GET /api/status` after startup. Confirm all three actual CC1101 modules are detected and receive stays active.
2. With the unit powered off, disconnect one CC1101, then power it on. The other connected radios must continue receiving. An absent module must not receive SPI clocks after CHIP_RDYn timeout.
3. Power off, reconnect the radio and power on. Check presence, recovery logging and RX counters. **Do not unplug live radio modules** for this test.
4. Provoke a reversible 868 RX interruption through controlled software diagnostics rather than introducing electrical faults. Confirm it re-arms and subsequent valid test frames are still received. Check 433 RMT rearm after a failed capture using a dedicated test fixture.
5. During any authorized experimental TX, verify the watchdog never issues a probe or stop during `transmitting` or `switching`. Check exact RX profile restoration and errors afterward.
6. Repeat with RFLink Builder device-specific USB/OTA test installation and check that OTA rollback/boot health still works. Do not enable TX or pairing buttons until real hardware transport tests prove safe and reliable.

Host tests compile the RX watchdog policy, exercise three manager-controlled receiver transitions, simulate a CC1101 868 IDLE recovery and three consecutive SPI errors, and verify shared CS handling. GitHub CI additionally compiles the actual ESP-IDF firmware. Neither host simulation nor compiling proves hardware readiness.
