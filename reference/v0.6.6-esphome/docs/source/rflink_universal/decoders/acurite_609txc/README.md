# Acurite 609TXC

Acurite 609TXC/609A1TX 433.92 MHz temperature and humidity; additive checksum; receive only.

- **Protocol:** OOK PWM 40-bit; additive checksum over four bytes; temperature and humidity.
- **Source:** https://github.com/merbanan/rtl_433/blob/master/src/devices/acurite.c
- **Identity:** Sensor ID resets after battery power cycle; candidate is not auto-merged.
- **Status:** Source researched and synthetic C++ host regression tested. Not yet verified with an actual physical sensor/CC1101.
- **Transmission:** deliberately disabled.
- **Frequency:** only the 433.92MHz RAW receiver; not Somfy 433.42 or Itho 868.30.

This code is independently written from protocol facts; the referenced rtl_433 implementation is a technical specification, not a copied decoder.
