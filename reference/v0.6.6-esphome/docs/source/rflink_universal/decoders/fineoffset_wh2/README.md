# Fine Offset WH2

Fine Offset WH2 433.92 MHz temperature and humidity; 48-bit preamble+payload CRC; WH2 only.

- **Protocol:** OOK PWM; 0xFF preamble; 40-bit data; CRC-8 poly 0x31 init 0x00. WH2A/WH5 deliberately excluded.
- **Source:** https://github.com/merbanan/rtl_433/blob/master/src/devices/fineoffset.c
- **Identity:** Sensor ID may change on power up; never auto-merge.
- **Status:** Source researched and synthetic C++ host regression tested. Not yet verified with an actual physical sensor/CC1101.
- **Transmission:** deliberately disabled.
- **Frequency:** only the 433.92MHz RAW receiver; not Somfy 433.42 or Itho 868.30.

This code is independently written from protocol facts; the referenced rtl_433 implementation is a technical specification, not a copied decoder.
