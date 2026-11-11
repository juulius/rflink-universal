# RFLink Universal v0.5.4 Radiolove — CRASHTRACE1

## Doel voor de gebruiker
Wanneer RFLink onverwacht opnieuw opstart wil je in **Logboeken → Systeem → Crashes en herstarts** niet alleen de reden zien, maar ook de beschikbare technische crashdetails: core, program counter (PC) en ruwe backtrace-adressen. Open de crashregel om de details te bekijken. Een bewaarde terugtrace kan met het juiste `firmware.elf` naar bronregels vertaald worden.

## Wat nieuw is
- Integreert de ESPHome-crashlogger via de **publieke log-callback-interface**, zonder een eigen panic handler of toegang tot ESPHome's private `.noinit`-geheugenlayout.
- De component vraagt tijdens ESPHome-codegeneratie een extra logger-listenerslot aan. Dat voorkomt dat het vaste callback-array ongemerkt vol raakt.
- Een kleine, vaste NVS-ring bewaart **maximaal vier technische crashrapporten** (alleen bij nieuw geconstateerde crashes). De bestaande acht resetrecords blijven intact. Geen logging van voortdurend ontvangen RF-ruis naar flash.
- Een rapport bevat reden, core, PC, maximaal zestien stackadressen per core en of ESPHome de crash aan de **huidige build** heeft kunnen koppelen.
- Een crash uit een andere firmwarebuild krijgt geen ten onrechte toegekend bootnummer of verkeerde symboolnamen. De juiste *oorspronkelijke* ELF blijft nodig voor de herleiding.
- Logviewer en diagnose-export tonen de aanvullende crashdata alleen wanneer die ook werkelijk aanwezig is. Oude NVS-resets zonder backtrace worden niet kunstmatig aangevuld.
- Korte zichtbare firmwareversie **v0.5.4 Radiolove** met unieke 12-tekensbronhash onder Technische details.

## Grenzen en veiligheidsmaatregelen
- ESPHome bewaart normaliter de laatst geregistreerde raw crash in `.noinit`; **historische** backtraces die vóór deze firmware niet zijn vastgelegd, kunnen niet worden teruggehaald.
- Na stroomonderbreking kan de nog niet overgenomen ESPHome-crashrecord verloren zijn. De vier eerder in NVS opgeslagen technische records blijven dan wel behouden.
- Dit mechanisme vangt een ESPHome-panic op als ESPHome zelf die tijdens de vorige boot heeft vastgelegd. Niet elke reset (zoals volledig stroomverlies) levert een backtrace.
- Dit is een aanvulling op logging, **geen oplossing van een panic of Orcon-TX-fout**.
- De vaste geheugencapaciteit blijft klein en er wordt geen volledige ESPHome-logger of secret-data bewaard.

## Installatie en controle
1. Behoud jouw eigen YAML/pinout, 6 MHz SPI en experimentele TX-labopties. Gebruik `UPDATE_RFLINK.bat` wanneer je de bestaande persoonlijke configuratie wilt behouden.
2. Controleer na build in `/rflink/api/status` de velden `crash_traces` en `crash_trace_persisted`, naast `boot_history`.
3. Open Logboeken → Systeem → Crashes en herstarts en klik op een gebeurtenis met **Backtrace beschikbaar**.
4. Bewaar bij toekomstige crashes ook de **firmware.elf die bij de gecrashte build hoort**, zodat PC/BT-adressen correct kunnen worden gedecodeerd. De bronhash is géén ELF-hash of symboolbestand.

## Tests / status
- Pure C++-parser: syntaxtest en scenario's met huidig/ouder firmwarebuildlabel.
- Browserproeven: gekoppelde en niet-koppelbare crashbacktraces in de historische logtijdlijn, op desktop en mobiel.
- Bestaande profiel-, radio- en koppelcode is niet functioneel gewijzigd.
- Niet uitgevoerd in deze omgeving: een volledige ESPHome/ESP-IDF-hardwarecompile, fysieke crashinjectie en NVS-persistentietest op het echte board. Werkende capture op het board is daarmee nog **niet bewezen**.
