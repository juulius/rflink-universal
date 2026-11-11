# RC4-HF3 LEDFIX — correctie voor ingebouwde WS2812

De webserver start nu op het echte bord. De vorige configuratie stuurde de
WS2812 echter aan via GPIO47. Voor de getoonde ESP32-S3 N16R8 met CH343P
en dubbele USB-C is GPIO48 de gebruikelijke LED-datapin. De GPIO47-vermelding
op sommige pinoutplaten is aantoonbaar onjuist voor veel revisies. Controleer
de hardware indien de LED nog niet werkt: een solder jumper kan 47/48 kiezen.

**Wijzigingen:** hoofd-YAML substitutions status_led_gpio GPIO47 -> GPIO48,
consistent bijgewerkte packagecommentaar, documentatie, pinoutregressietest
en Windows release-guard. Alle andere ESPHome runtimebestanden ongewijzigd.

**Bestaand device:** behoud secrets.yaml, importeer nieuwe bundle, pas
status_led_gpio in de daadwerkelijk actieve hoofd-YAML aan naar GPIO48 (ook
wanneer ESPHome voor dat bestand 'behouden' kiest), valideer en voer OTA uit.
De release is geen vooraf gecompileerde firmware.

**LED-controle:** LED-instellingen in /rflink -> helderheid op 100%,
'Groene hartslag uit bij gereed' uitschakelen, twee hartslagen per 6 seconden
bekijken. Controleer anders logs op esp32_rmt_led_strip of no free TX channels.
Geen hardware of echte ESPHome firmwarecompile beschikbaar in testomgeving.

**Opgelet:** de bestaande release-maker kiest bewust de nieuwste updater-output.
Als die nog GPIO47 bevat, meldt de release-maker een waarschuwing.
Controleer bij een nieuwere update-output altijd de effectieve pinout;
GPIO47 mag bewust gekozen worden op een aangepaste PCB met soldeerbrug.
De standaard-HF3-LEDFIX-release gebruikt GPIO48.

## LED-bedieningscorrectie (2026-09-18)

Het eerdere webformulier had de negatieve optie **Groene hartslag uit bij gereed**.
Een vinkje betekende daardoor UIT. In deze update is dit omgezet naar
**Groene hartslag bij gereed**: vinkje AAN betekent hartslag AAN. De bestaande
opslag (`quiet` in NVS en `/rflink/api/status`) blijft ongewijzigd en de
webinterface keert de waarde bij lezen en opslaan om. Bestaande voorkeuren
blijven behouden. Correctie van de verouderde GPIO47-tekst in de webinterface.

De hartslag is uitsluitend zichtbaar bij `gereed`; knippert twee keer per zes
seconden. Stel voor de eerste test helderheid op 100% en klik op Opslaan.
Wanneer `/rflink/api/status` de LED-status `fout`, `verbinding_verloren` of een
andere hogere-prioriteitstatus toont, schakelt de gereedhartslag terecht niet in.
De firmware-LED-statusmachine en hardwarepinnen zijn niet aangepast.
