**Actuele decoder-TX status:** Nexa, Byron, Dooya, RC-Switch, Somfy RTS en Itho RFT zijn hostgetest. Somfy gebruikt een native virtual-remote/pairing-pad met persistente rolling counter. Itho gebruikt native 868,30MHz packet-TX met per-device virtuele zenderstate en pairing/unpair. Zie `RELEASE_NOTES.md`. Hieronder staat ook de historische TX1/HA-uitleg.

# RFLink TX1 – gecontroleerde HA-bediening (testrelease)

RFLink blijft de bron van waarheid. Alleen goedgekeurde, opgeslagen Nexa-apparaten
met werkelijk ontvangen 433,92MHz-code, geldig kanaal/groep/niveau en expliciete
zendtoestemming krijgen Home Assistant **button**-entiteiten Aan/Uit.

## Activeren
1. Configureer de bestaande Mosquitto-verbinding zoals in INSTELLINGEN.md.
2. `ha_tx_controls` is de historische hoofdschakelaar voor ingebouwde TX en staat in deze developmentrelease standaard `true`. Zet hem op `false` om alle ingebouwde TX te blokkeren.
3. Compileer/flash, leer een Nexa-apparaat op radio 2 in, publiceer naar HA.
   Oude opgeslagen Nexa-apparaten zonder gevalideerd `tx_radio` moeten opnieuw
   een RF-signaal sturen; keur `tx_radio` daarna onder Functies beheren goed.
   Pas dan kan zendtoestemming worden gegeven.
4. Open RFLink -> Apparaten -> 'HA-bediening toestaan'; bevestig expliciet.
5. De HA-buttons 'Aan'/'Uit' worden via MQTT Discovery aangemaakt. Deze zijn
   **momentary buttons**, geen status-schakelaars: zendopdracht != fysieke status.

## Beveiligingsregels
- De hoofdschakelaar staat in deze developmentrelease aan, maar HA-TX blijft per apparaat UIT tot expliciete toestemming. Er wordt nooit automatisch op basis van een ontvangen frame gezonden.
- Alleen Nexa, vaste hoge-identiteitszekerheid, precies één identiteit,
  bekende radio 433,92MHz, kanaal/groep/niveau, goedgekeurde actie on/off.
- Geen KeeLoq of ongecontroleerde RAW-replay via dit pad. Somfy gebruikt zijn eigen encoder, virtual-remote-state en pairingflow; PROG wordt niet als normale HA-knop gepubliceerd. Itho gebruikt zijn native packetencoder; normaal verschijnen Low/Medium/High en timeracties pas na pairing + expliciete HA/TX-toestemming.
- MQTT-commandosessies vernieuwen bij iedere brokerherverbinding om oude
  retained commandopayloads te weigeren. HA publiceert commandopayloads niet retained.
- RFLink controleert apparaat, expliciete toestemming, radioaanwezigheid en
  rate-limit (max. één aangevraagde TX per 600 ms) vóór verzenden.
- Gebruikers die de RFLink-webpagina buiten het vertrouwde LAN aanbieden, moeten
  toegang zelf beveiligen: de bestaande firmware heeft geen eigen gebruikerslogin.
- De terugmelding `TX aangevraagd` is GEEN bevestiging door het RF-eindapparaat.
- Oude NVS-versie 1 blijft leesbaar: toestemmingsbit 0x08 wordt toegevoegd
  binnen hetzelfde bestaande flags-byte, standaard uit voor oude records.

## Teststatus
Hostpolicy en MQTT-integratietests zijn uitgevoerd; een
volledige ESPHome 2026.9.0 compile, echte 433,92MHz-signalen en MQTT/HA
praktijktest zijn vereist voor productiegebruik.

## Let op: RFLink is geen statusmeting
Een verzonden opdracht resulteert in een *button*, geen aan/uit-*switch*. Een
werkelijke terugmelding van het eindapparaat is vereist voor status. Zorg dat
Mosquitto ACL/toegang beperkt blijft tot vertrouwde clients en expose de
RFLink-webinterface niet rechtstreeks aan internet.
## Itho RFT vanaf ITHO1

1. Leer een `IthoRFT`-apparaat in via de normale wizard.
2. Open het apparaat en kies **RFLink koppelen** terwijl de ventilatie-unit in de registratieperiode staat. Gebruik compatibility-pairing alleen onder Advanced als de normale variant niet werkt.
3. Na pairing kan de webinterface **Laag, Midden, Hoog, Timer 10, Timer 20 en Timer 30** sturen. Stand-by en ontkoppelen staan onder Advanced.
4. Voor Home Assistant: voeg het apparaat aan HA toe en geef daarna expliciet HA-bediening/TX-toestemming.

RFLink maakt een eigen virtuele 24-bit Itho-zenderidentiteit; een ontvangen afstandsbedienings-ID wordt niet hergebruikt. De logische verzendteller wordt persistent vooruitgeschreven voordat het pakket wordt verzonden en kan via restore niet achteruit. De oude globale YAML-ID en losse ESPHome-Itho-templateknoppen bestaan in ITHO1 niet meer. Softwaretests dekken pairing, unpair, speed/timers, packet-decode, persistence, restore, hardware guard en MQTT-dispatch; een echte Itho-unit blijft de uiteindelijke hardwaretest.

