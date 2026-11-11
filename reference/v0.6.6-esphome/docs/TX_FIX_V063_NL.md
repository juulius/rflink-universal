# v0.6.3 Radio Gaga — TX-registercorrectie (testbuild)

## Bevestigd in diagnoselog #130

RAMSES 1FC9 TX mislukt voordat RF wordt verzonden: CC1101 DEVIATN register 0x15 vraagt 0x48, chip leest 0x40 terug. Bit 3 is gereserveerd en leest als 0.

## Wat is aangepast

- De 868 MHz RAMSES GDO-UART preset gebruikt 0x40 in plaats van 0x48.
- De twee alternatieve RAMSES-labpaden gebruiken eveneens 0x40, inclusief verificatie.
- De GDO-UART adapter weigert presets met gereserveerd DEVIATN-bit 3.
- De host-test simuleert het hardwaregedrag van het gereserveerde bit.

## Nog NIET bewezen / NIET aangepast

- Dit is een gerichte TX-registerpatch, geen bewezen volledige zendoplossing.
- GPIO16 (UART TX / CC1101 GDO0) en de ESPHome packet-interrupt moeten na TX op hardware worden gecontroleerd.
- Een succesvolle UART-overdracht is geen bewijs dat een RAMSES-koppeling door een ventilatiebox is geaccepteerd.
- Geen native ESP-IDF-migratie in deze patch.

## Testvolgorde op hardware

1. Maak back-up van de bestaande instellingen en upload de testbuild.
2. Controleer dat alle drie radio's normaal ontvangen en noteer RX-ruisevents per radio.
3. Start één koppelactie. Controleer of de 0x15-registerfout verdwenen is en TX-drain wordt gelogd.
4. Controleer daarna actief ontvangst op 868,30 MHz en beide 433 MHz-radio's.
5. Als 868 RX stilvalt: herstart het board en exporteer direct opnieuw diagnose. Niet verder koppelen totdat GPIO/IRQ-herstel gerepareerd is.
