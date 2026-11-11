# Rubicson temperature

Rubicson / older TFA 30.3197 temperature (36-bit frames); CRC-8; receive only. Some InFactory/PT-310 devices send 37/38 bits: not validated.

- **Protocol:** OOK PPM 36-bit; 500us marks; 1000/2000us gaps; CRC-8 poly 0x31 init 0x6c.
- **Source:** https://github.com/merbanan/rtl_433/blob/master/src/devices/rubicson.c
- **Identity:** ID can change on battery reset; channel contributes to identity but is not globally unique.
- **Status:** Source researched and synthetic C++ host regression tested. Not yet verified with an actual physical sensor/CC1101.
- **Transmission:** deliberately disabled.
- **Frequency:** only the 433.92MHz RAW receiver; not Somfy 433.42 or Itho 868.30.

This code is independently written from protocol facts; the referenced rtl_433 implementation is a technical specification, not a copied decoder.
