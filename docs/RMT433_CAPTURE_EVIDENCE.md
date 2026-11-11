# 433 MHz RMT capture evidence (native v0.7)

## Evidence from 11 November 2026

The supplied `RFLink_Logbook (6).csv` contains two system events on radio 1
(433.92 MHz), code 35, `value=2`, `count=1` at uptime 120883 ms
and 210893 ms. Each is the *30-second aggregate notification* of a full
RMT host capture. It is **not** a decoded protocol frame, and the
notification uptime need not match the RF arrival time. This is not proof
that the CC1101 failed or that a valid button transmission was lost.

Native v0.7 uses ESP32-S3 non-DMA RMT with 96 hardware symbols and a
separate 512-symbol host receive buffer. A completion with
`num_symbols >= 512` is deliberately rejected as incomplete: neither a
protocol decoder nor the raw RF detail archive receives that capture.
Continuous noise and a legitimate long pulse train can both fill it.
Increasing the buffer without a controlled comparison is not a root-cause
fix.

## New evidence fields and behavior

The counters remain per-radio and do not cross-contaminate PSRAM log rings.

- `rx43342_queue_drops` / `rx43392_queue_drops`: completion callbacks
  that could not enqueue an RMT result. A dedicated FreeRTOS SMP critical section protects the ISR increment
  and task-level status snapshot. The std::mutex is never used in the ISR;
  no logging or heap work occurs there.
- `rx43342_last_full_symbols` / `rx43392_last_full_symbols`: symbol
  count of the most recent rejected full capture on each receiver.
  Unlike `last_symbols`, this is not replaced by a subsequent short
  or valid capture.
- Existing `full_frames`, `short_frames`, `filtered_frames`,
  `forwarded_frames` and `capture_capacity` are preserved.
- System log code 35 retains `count` as the number of occurrences in
  the previous approximately 30 seconds. Reason values are:
  0 forwarded, 1 too short, 2 full/incomplete, 3 filtered, **4 completion
  queue overflow**. Both 2 and 4 are WARN. The UI distinguishes these
  outcomes; current status is explicitly not event-time evidence.

An RMT capture, a frame forwarded to the decoder and a successfully
decoded RF message are different stages. Even a complete capture is not
RF protocol validation.

## On-device acceptance check

1. Record firmware build, board profile and channel status; back up the
   logs before cycling power. Keep the proven v0.6.6 reference available.
2. With no intentional transmitter, record 60 seconds of idle counters
   on 433.42 and 433.92 MHz. Note ambient full captures and queue drops.
3. Use a controlled, known 433.92 MHz fixed-code transmitter at a fixed
   distance. Press 10 times separated by a few seconds. Record forwarded,
   full, filtered and queue counters **before and after**, plus raw pulse
   logs when available. Repeat with the same transmitter running v0.6.6
   on the same board and pinout.
4. Repeat on 433.42 MHz using a compatible known source if available.
   Do not mislabel out-of-band transmissions as evidence for this radio.
5. If full captures persist, inspect radio environment, pulse duration,
   GDO2 edges, RMT idle timeout, filtered pulse count, and concurrent
   worker load before changing memory or modem settings. If queue drops
   rise, investigate scheduling/ISR-to-worker delivery separately.
6. Gate any proposed buffer enlargement or decoder port on repeatable
   hardware evidence; host/CI tests alone cannot prove air reception.

No changes to CC1101 registers, GPIOs, OTA, packet decoder routing,
radio control, or 30% PSRAM detailed-log quotas are made here.
