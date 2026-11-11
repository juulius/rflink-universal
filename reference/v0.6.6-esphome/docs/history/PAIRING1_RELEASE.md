# PAIRING1 — universele begeleide apparaatinstallatie (veilige eerste implementatie)

## Gereed
- Alle 26 ingebouwde profielen hebben dezelfde declaratieve pairing-methode: `none`, `manual`, `receiver_learn`, `virtual_remote` of `unsupported`. Geen merkgebonden UI-implementatie.
- Elke opgeslagen apparaatkaart heeft een knop **Stap voor stap instellen**. De wizard toont de stappen één voor één, laat het profiel bevestigen en scheidt ontvangen, verzenden en handmatig gecontroleerde reactie.
- Bestaande Somfy RTS- en Itho RFT-koppelacties zijn via de wizard bereikbaar. De werkende centrale TX-veiligheidscontroles blijven leidend. Verzending wordt nooit gelijkgesteld aan bewezen apparaatreactie.
- RAMSES-II Orcon toont de stappen en technische reden waarom koppelen en zenden nog niet mogelijk zijn. De acht profielacties bestaan, maar blijven uit tot de fysieke 868-MHz-TX en volledige 1FC9-uitwisseling gevalideerd zijn.
- Eigen apparaatprofielen hebben nu bedieningsacties, keuze van koppelmethode en maximaal acht eigen instructiestappen, ook achteraf bewerkbaar. User profile metadata activeert nooit zelfstandig niet-beoordeelde TX-encoders.
- Oude eigen profielopslag in NVS snapshot v1 blijft leesbaar; de nieuwe versie slaat v2 met twee snapshots op en verandert geen apparaat-UID. Zowel oude snapshot-migratie als v2-round-trip zijn getest.
- Het filter **Koppelbare apparaten** toont nu ook apparaten waarvoor het gekozen profiel installatie vereist, zonder te beweren dat fysieke TX al klaar is.

## Nog niet gerealiseerd
- **Orcon daadwerkelijk koppelen en bedienen, ook vanuit Home Assistant, is NIET werkend**. RAMSES-II staat nog expliciet op TX disabled. Wel aanwezig: RAMSES-II-RX, codec voor commando's en 1FC9-pairing, guard op virtueel adres/ACK en PC-hosttests. Nog noodzakelijk: fysieke CC1101-TX op dit board, gecontroleerde radiolease, betrouwbare asynchrone RX voor het handshake-antwoord, eigen persistente virtuele controller-ID en fan-ID, hardwaretest en 868-MHz-regressie. UI mag deze status niet vervalsen.
- Firmware kan niet hier tegen volledige ESPHome/ESP-IDF en echte hardware worden gecompileerd/geflasht.
- Koppelstatus van andere apparaten kan na het zenden nog alleen **zonder RF-ACK** worden vastgesteld. In de wizard staat dat zichtbaar.
- User profile wizard is versie 1: declaratieve instructies, knoppen, methoden; nog geen programmeerbare `expect RX frame`, branching/timers of firmware-side recovery journal.

## Installatie
Aanbevolen: PATCH ZIP over de exacte PROFILECATALOG2 SOURCE, dan `MAAK_RELEASE.bat` en ESPHome compileren. De PATCH bevat geen YAML, dus behoudt je `cc1101_spi_rate_hz: "6000000"`. Bij FULL SOURCE of DEFAULT bundle moet je jouw eigen YAML opnieuw overnemen (standaardversie gebruikt nog 200000 Hz). Gebruik bestaande werkende firmware als OTA-backup.

## Tests
- 26 ingebouwde profielen met declaratieve koppelmethode en 4 talen
- C++-compiler-syntaxis voor UI/firmware in hoststubs
- v1->v2 persistentie-migratie voor eigen profielen, knoppen-/stepvalidatie
- Chromium-desktop 1300px + mobiel 390px: Orcon-wizard kan geen TX triggeren, Itho-stappen wel bereikbaar
- Decoder/API v2 catalogus, ESPHome-achtige headercompile, pakketintegriteit, bestaande pinout
- Volledige ESPHome/ESP-IDF-build en fysieke CC1101/ventilatieboxproef **niet uitgevoerd**
