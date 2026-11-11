# RAMSES3 – veilige 868-MHz-radiomodus (fundering)

## Wat is nu gebouwd?

Een **hardware-onafhankelijke** radiomoduscoördinator met een exclusieve lease op radio 2 (868,30 MHz). De coördinator zit in de RFLink-kern en blokkeert Itho-pakketverwerking en pakket-TX zolang de radio door een ander protocol geclaimd wordt. Ook de SPI-watchdog mag geen parallelle registerprobe doen. Alle acties verlopen vanuit de normale event loop, zonder wachtlus of automatische TX.

**Deze versie schakelt fysiek nog niet naar RAMSES-II**: er is bewust nog geen `Backend` gekoppeld. In de standaardconfiguratie blijft de bestaande Itho-configuratie volledig actief, ook na OTA.

## Gecontroleerde toestanden

`ITHO` → `ACQUIRING` → `RAMSES_RX` → `RESTORING` → `ITHO`.

Bij een mislukking: `RESTORING` → `FAULT`; terugkeer naar Itho wordt pas gemeld als alle fysieke stappen én de verificatie zijn geslaagd. Een beschadigde radio blijft softwarematig geblokkeerd tot een expliciete recovery. Lease-tijd maximaal 30 seconden en `millis()`-rollover veilig.

## Nog nodig voor activering (hardwarebackend)

1. **Exclusieve SPI-eigenaar:** packet-driver ISR/loop en bestaande `run_radio_watchdog` pauzeren voordat een register wordt gewijzigd. Niet gelijktijdig dezelfde CC1101 vanuit twee componenten bedienen.
2. **Snapshot/harde restore:** leg relevante CC1101-registers en GPIO/interruptstatus vast, inclusief GDO0/2 routing. `SIDLE`, alleen vanuit juiste toestand `SFRX/SFTX`. Bij restore liefst ESPHome's eigen `reset()`/`configure()` gebruiken en de resulterende configuratie via readback vergelijken met Itho-profiel.
3. **RAMSES RX:** CC1101 2-FSK, ~38.4 kbps async raw bitstream, GDO2→ESP32-S3 UART RX; parsing met checksum/Manchester + framing. Geen 63-byte FIFO om RAMSES voor te wenden.
4. **RAMSES TX:** GDO0→UART TX met frame timing; poll op einde TX; RX-pakketvenster voor `1FC9`-ACK. In beginsel alleen na expliciete pairing-activering, unieke opgeslagen eigen remote-ID en goedgekeurde RX/TX-controles.
5. **Hardwarebeveiliging:** herstel na SPI-fout, UART-fout, TX-timeout, pairing-timeout en ESP32-reboot; eenduidige arbitrage voor OTA en watchdog. Geef nooit `paired=true` bij alleen een TX-succes.
6. **Verificatie:** eerst register-readback zonder zenden, dan receive-only Orcon en gecontroleerd Itho op drukknoppen. Pas daarna TX vrijgeven, daarna pairing en functionele bediening. Meet radiochannel-bezetting; plan geen lange RX-only wissels zonder waarschuwing over gemiste Itho- of Orcon-pakketten.

## Waarom eerst deze veilige scheiding?

De standaard ESPHome CC1101-driver beheert zelf GDO0-interrupts, de 63-byte FIFO en een interne configuratiekopie. Een eigen UART-driver die onafhankelijk registers schrijft, kan die driver desynchroniseren. De state machine voorkomt concurrerende radio-acties, maar de fysieke backend moet aantoonbaar pauzeren, herstellen en verifiëren vóór activatie.

## Teststatus

- Hosttest: fail-closed zonder backend, geen gelijktijdige aanvragen, watchdog/TX-guards, verlenging en expiratie, mislukte overgang, mislukte restore, handmatige recovery en 32-bit rollover.
- Niet gevalideerd: echte SPI/UART-switch, RX over echte CC1101, ESPHome compile op ESP32-S3 en radiotransmissie. De koppelknop en RX/TX Orcon blijven daarom **uit** in deze source-release.

Bronnen: TI CC1101 datasheet (`SIDLE`/`SFRX`/`SFTX`); ESPHome native CC1101 component; wlcrs/esphome-ramses `cc1101_multiplexer`; me-processware/orcon-15rf-controller (MIT).
