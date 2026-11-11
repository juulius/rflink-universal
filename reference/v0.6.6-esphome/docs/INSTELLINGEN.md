# RFLink Universal – alle gebruikersinstellingen op één plaats (CONFIG1)

Open **`rflink-universal.yaml`** en ga naar de eerste sectie `substitutions:`. Alle normale instelbare waarden staan bovenaan. Meteen daaronder staat `packages:` om de optionele MQTT-functie eenmalig in te schakelen. Wijzig geen radio-RMT-budget, radiodrivercode of scriptlogica daaronder om een persoonlijke instelling te veranderen.

## MQTT met jouw bestaande Mosquitto (eenmalig)

Een extra broker is niet nodig. Controleer eerst in Home Assistant onder **Instellingen → Apparaten en diensten** of de MQTT-integratie met jouw bestaande Mosquitto is verbonden. RFLink moet dezelfde broker via het IoT-netwerk kunnen bereiken; een hostnaam moet vanuit dat netwerk werken.

1. Zoek in het hoofd-YAML onder `substitutions` naar `mqtt_broker: "VUL_IP_OF_HOSTNAAM_IN"`. Vervang dat met de **hostnaam of het IP-adres van de bestaande Mosquitto-server**, bijvoorbeeld `"192.168.178.50"` (dit is een voorbeeld, niet jouw bekende IP). Er bestaat geen afzonderlijk verplicht veld "broker name".
2. Zet `mqtt_port` op de werkelijke luisterpoort van Mosquitto: meestal `1883` zonder TLS. Gebruik `8883` alleen als TLS daadwerkelijk geconfigureerd is; deze CONFIG1-release configureert geen TLS en is bedoeld voor een vertrouwd thuisnetwerk. Stel dus niet zomaar `8883` in.
3. Zet `mqtt_username` op een bestaande Mosquitto-gebruiker, bij voorkeur een apart account voor RFLink. De inlognaam is niet hetzelfde als de apparaatnaam.
4. Voeg in jouw **eigen** `/config/esphome/secrets.yaml` de regel `rflink_mqtt_password: "JOUW_ECHTE_WACHTWOORD"` toe. De werkelijke waarde blijft buiten de broncode en releasebestanden. Gebruik geen wachtwoord uit een voorbeeldbestand.
5. Zoek direct onder `substitutions:` de sectie `packages:` en verwijder alleen het `#` vóór `ha_mqtt: !include rflink_universal/options/ha_mqtt.yaml`. Alle spaties laten staan.
6. ESPHome **Validate**, daarna **Compile**, pas daarna zelf OTA-installeren. Controleer vervolgens MQTT-status in de RFLink-webinterface. De native ESPHome API blijft aan.

De optionele include leest `mqtt_broker`, `mqtt_port` en `mqtt_username` uit de labels bovenaan. Het wachtwoord leest hij alleen uit `secrets.yaml`. Standaard is MQTT uit, dus de firmware vereist bij een schone standaardconfiguratie geen MQTT-account. De releasecontrole weigert een *ingeschakelde* MQTT-package als het brokeradres nog de invultekst bevat.

**Discovery:** ESPHome's eigen MQTT entity discovery staat doelbewust op `false`, omdat het board zelf al via de native API gekoppeld is. RFLink publiceert de goedgekeurde RF-apparaten met eigen Discovery-berichten onder de vaste `homeassistant`-prefix. Die prefix en de apparaatidentiteit zijn geen gewone instelbare labels: veranderen zonder migratie kan dubbele of verweesde entiteiten opleveren.

**HA2-beheer:** MQTT één keer verbinden is voldoende. Tijdens het bevestigen van een nieuw apparaat is het HA-vinkje standaard aan als MQTT verbonden is en de decoder ondersteunde functies levert; je kunt het uitzetten. Nieuwe functies van een al opgeslagen apparaat verschijnen uitsluitend als voorstellen onder **Apparaten → Functies beheren** en worden pas na goedkeuring geregistreerd/gepubliceerd. Verwijderingen vereisen bevestiging; bekende sensorwaarden en knopacties worden live bijgewerkt. Voorstellen zijn vluchtig tot goedkeuring. Zie `START_HERE.md` en `RELEASE_NOTES.md`.

## Wat staat er centraal?

| Groep | Labels | Gedrag bij update |
|---|---|---|
| Apparaatidentiteit | `name`, `friendly_name` | Oude identiteit standaard behouden. Wijzig `name` niet onnodig: HA MQTT unique IDs zijn eraan gekoppeld. |
| PCB en LED | `board_revision`, `spi_*`, `radio*_csn`, `radio*_gdo*`, `status_led_gpio` | Pinout in één geheel selecteren; PCB fysiek controleren. |
| WiFi | `wifi_ssid`, `wifi_password`, `wifi_fallback_ssid`, `wifi_fallback_password` | WiFi-wachtwoorden via bestaande secret-namen. |
| MQTT | `mqtt_broker`, `mqtt_port`, `mqtt_username`, include onder `packages` | Alleen actief na opt-in; lokaal invullen. Wachtwoord blijft secret. |
| ESPHome toegang | `api_encryption_key`, `ota_password` | Bestaande sleutels behouden. Geen ongemerkte OTA-encryptiemigratie. |
| Optionele voorkeuren | `log_level`, `rf_dedupe_window`, `rf_candidate_threshold` | Per sleutel vergelijken; afwijkingen op compatibiliteit controleren. |

**Waarom staan niet alle getallen uit de firmware bovenaan?** Radiofrequenties en decoderprofielen zijn op meerdere plaatsen aan protocolcode gekoppeld. RMT-geheugen en de CC1101-registerinstellingen zijn hardware-/firmwaregrenzen, geen veilige individuele gebruikersinstellingen. Ze veranderen alleen in een geteste firmware- of hardwareprofielrevisie. Dat voorkomt schijnbaar werkende maar inconsistente instellingen.


### Itho RFT vanaf ITHO1

Itho gebruikt geen globale `itho_id1/2/3`- of `itho_device_type`-instellingen meer in de hoofd-YAML. Na het inleren krijgt ieder Itho-apparaat een eigen virtuele RFLink-zenderidentiteit en teller in de apparaatopslag. Koppel RFLink daarna één keer als extra afstandsbediening. Een installatie die de oude losse ESPHome-Itho-knoppen gebruikte, moet na de overstap op ITHO1 opnieuw koppelen; de oude globale zender-ID wordt niet stilzwijgend aan een ingeleerd apparaat toegewezen.

## Updater

De updater vergelijkt elk afzonderlijk `substitutions`-label met de passende gecontroleerde baseline en nieuwe release. PCB-pinnen blijven één groep. De include `ha_mqtt` is een **functionele keuze** en wordt niet automatisch aangezet als de nieuwe DEFAULT standaard uit staat. Bij twee onverenigbare wijzigingen stopt de updater: oude firmwareblokken worden nooit blind meegenomen. Voor een herkende, eerder ingeschakelde HA1-configuratie behoudt de updater de bestaande broker- en gebruikersnaam-secretverwijzingen in de nieuwe labels en houdt hij de HA-package aan. Bij een schone DEFAULT of handmatige migratie vul je het brokeradres zelf in. Verifieer de optionele MQTT-package en de labels vóór Compile; een CUSTOM-bundle met de invultekst voor de broker wordt geweigerd.

**Privacy:** deel jouw `secrets.yaml` nooit. De voorbeeldsecrets zijn alleen een lijst met vereiste sleutelnamen. De release bevat geen ingevuld broker-IP, geen wachtwoorden en geen sleutels.


## Optioneel: HA-bediening met TX1

`ha_tx_controls` is de historische naam van de hoofdschakelaar voor ingebouwde TX.
In deze developmentrelease staat hij standaard op `true`; er wordt daardoor nog steeds
nooit automatisch gezonden. Somfy en Itho vereisen expliciet koppelen; Home Assistant TX
vereist daarnaast een opt-in per apparaat. Gebruik MQTT met je bestaande
Mosquitto-broker wanneer je ingebouwde bediening ook in Home Assistant wilt.
Daarna in RFLink per apparaat 'Toevoegen aan HA' en 'HA-bediening toestaan'
bevestigen. Zet `ha_tx_controls: false` alleen als je alle ingebouwde TX volledig
wilt uitschakelen. RFLink zendt nooit alleen omdat een RF-signaal is ontvangen.
Meer informatie en beperkingen: `TX1_BEDIENING.md`.
