# RFLink Universal v0.5 Radiolove – PAIRRADIO9

## Doel vanuit de gebruiker

Bij een knop op een Orcon- of ander RF-apparaat wil ik in **Logboeken → Radio** kunnen zien:
1. wat de firmware probeert te verzenden;
2. welke radio/frequentie is geselecteerd;
3. of de router/driver de verzending accepteert of afwijst;
4. waarom een opdracht mislukt; en
5. of de radiodriver zijn werk en RX-herstel als voltooid meldt.

De **Weblog** bewaart de knop-/API-actie en de **Systeemlog** de firmwarebeslissingen. De radiozending zelf (inclusief afwijzingen) hoort in de **Radiolog**.

## Wijzigingen

- `ramses_send_management_`: het 1FC9-aanbod en de 1FC9-/10E0-vervolgberichten krijgen fysieke TX-stappen in de Radiolog; het zendplan draagt de bestaande frequentie `868299900` Hz.
- Een geweigerde TX-route logt de fout plus diagnostiek over radio-toestand, beschikbaarheid en UART-gereedheid.
- `ramses_pair_start`: logt het koppelverzoek *voordat* de controles worden uitgevoerd; een afwijzing wordt gelogd inclusief reden.
- RAMSES 10E0-test en AUTO-test: afwijzingen verschijnen ook in de Radiolog.
- 1FC9-koppelantwoord en het uitblijven daarvan worden gelogd.
- Het protocolonafhankelijke `dispatch_native_plan_` registreert TX-aanvraag, selectie, uitvoeringspoging, fouten en driverresultaten voor overige native TX-routes.
- TX-gebeurtenissen komen in dezelfde begrensde **radio-signaalbuffer** als RX. Geen vierde onafhankelijke logbuffer, geen kopie in de ruisbuffer. De bestaande instelbare limieten gelden dus ook voor TX-gebeurtenissen.
- De logviewer herkent `direction: TX`, `level: error`, `identity` en de detailtekst. Radio-TX-fouten worden zichtbaar bij filter ‘Fouten’ en wanneer ruis is uitgeschakeld.
- UI-fouten bij de koppelwizard worden leesbaar gemaakt en in de Weblog vastgelegd; de Systeemlog houdt de firmwarebeslissing bij.

## Voorbeeld (illustratief, geen echt ontvangen data)

```text
Radio  868,30 MHz  Info   TX · RamsesII · Koppelverzoek aangevraagd: 1FC9
Radio  868,30 MHz  Info   TX · RamsesII · TX voorbereiden: 1FC9 offer
Radio  868,30 MHz  Fout   TX · RamsesII · TX afgewezen: 1FC9 offer
                            Route unavailable/busy; radio=itho_rx, serial_ready=no
Radio  868,30 MHz  Fout   TX · RamsesII · Koppelverzoek mislukt: 1FC9
```

Er wordt uitdrukkelijk niet gesuggereerd dat een **radio-opdracht** daadwerkelijk is uitgezonden indien de router deze heeft geweigerd. Een positief driverresultaat is bovendien geen bevestiging van uitvoering door het externe apparaat.

## Getest

- C++ `RFLinkWebHandler` syntax-only met host-stubs: PASS.
- Browser (1440 en 390 pixels): Radio-TX-aanvraag, foutniveau, radiofilter, ruisfilter en detailregels: PASS (gesimuleerde events).
- Bestaande gecombineerde logboektest (1440 en 390 pixels): PASS.
- RAMSES-II TX codec, 8 commando's / ACK-protocol: PASS.
- Orcon-wizard browserregressie en eerdere Pairing-test: PASS.
- DIAG7 browserregressie (1440 en 390 pixels): PASS.
- Release-modus, pakketintegriteit en PACKFIX1 guards: PASS.

**Niet gevalideerd:** complete ESPHome/ESP-IDF-compile, echte CC1101-verbinding, fysiek TX-signaal, ontvangst ACK en werkelijke Orcon-bediening. Dit blijft een experimentele release.

## Installatie

Gebruik jouw persoonlijke YAML via `UPDATE_RFLINK.bat`; behoud de 6 MHz SPI, de drie radio-GPIO-configuraties en de lab-flags. Na flash moet `firmware_revision` eindigen op `DIAG7-PAIRRADIO9`. Bij haperende UI: Ctrl+F5. Test handmatig één 1FC9-koppelpoging en exporteer **Logboeken → Radio** met 868,30 MHz en *Ruis tonen* uit.

Dit pakket laat de decoder, het koppelprotocol en de veiligheidscontroles intact: het logt meer en bevat daarnaast de eerder geconstateerde ontbrekende frequentie voor 1FC9-managementframes.
