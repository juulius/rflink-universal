# RFLink Universal v0.5.5 Radiolove — Diagnose & TX-queue

## Wat is nieuw

- **Logboeken → Diagnose** is de enige centrale plek voor alle bestaande diagnoses: boot- en crashgeschiedenis, beschikbare backtraces, status van de drie CC1101-radio's, SPI-fouten en herstelpogingen, geheugenmetingen, en de experimentele GDO/UART-TX-test.
- De bestaande logboeken **Alles, Radio, Systeem, Web** blijven ongewijzigd, met dezelfde vaste hoogte, filters, export en tijdstempels `uu:mm:ss,mmm`. Diagnose vervangt de tijdlijn in plaats van eronder te verschijnen.
- Het Diagnose-tabblad begint met vier compacte kaarten. Details en experimentele knoppen worden alleen na een klik zichtbaar. Historische resets zonder tijdstempel tonen bewust geen verzonnen tijd.
- **Instellingen & hulp** gebruikt dezelfde rustige kaartstijl; de apart geplaatste Diagnose- en TX-labpanelen zijn vanuit Instellingen verwijderd (oude diep-links leiden naar Logboeken → Diagnose). Bibliotheken en MQTT blijven in Instellingen.
- De radio-SPI-watchdog schrijft alleen **statusovergangen, SPI-fouten en herstelacties** naar de Systeemlog — niet iedere succesvolle controle van tien seconden. TX-aanvragen en afwijzingen blijven in de Radiolog.
- De experimentele **Orcon 1FC9-koppeling en Auto-test** worden vanuit een HTTP-aanvraag **in een eenmalige, begrensde wachtrij** gezet en vanuit de ESPHome-hoofdloop uitgevoerd. Een API-antwoord bevestigt uitsluitend *ingepland* — geen geslaagde verzending of reactie van de ventilatiebox. De hoofdloop controleert in ESP-IDF de beschikbare taakstack en weigert de proef bij te weinig ruimte. Andere universele TX-routes zijn nog niet omgezet.

## Belangrijke beperkingen

- **Niet op hardware bewezen:** de eerdere `vApplicationStackOverflowHook`-crash tijdens het koppelen is nog niet fysiek opnieuw getest. Dit is een gerichte wijziging van het verdachte HTTP-pad, geen garantie dat alle stackproblemen weg zijn.
- Een ESPHome-crashbacktrace verschijnt alleen wanneer ESPHome deze werkelijk heeft opgeslagen en RFLink kon overnemen. Er zijn maximaal vier technische crashrapporten beschikbaar; de NVS-bootgeschiedenis bewaart acht starts.
- Er is nog geen volledige ESPHome/ESP-IDF-firmwarebuild op de echte ESP32-S3 uitgevoerd. Gerichte C++-syntax- en browsertests zijn geen vervanging voor hardwarevalidatie.
- De DEFAULT YAML is bewust generiek. Gebruik **UPDATE_RFLINK.bat** voor je bestaande PCB-YAML met 6 MHz SPI, pinout en experimentele labopties. Controleer de updaterkeuzes en maak vooraf een kopie. De oorspronkelijke YAML mag niet gewijzigd worden.

## Hardwaretest

1. Controleer na flashen in **Instellingen → Over RFLink Universal** de versie `v0.5.5 Radiolove` en de bronhash.
2. Open **Logboeken → Diagnose**. Controleer de drie radio's, SPI-fouten en de laatste resets. Kijk of de vier panelen onafhankelijk openen.
3. Maak een diagnose-export vóór een test. Test met één expliciet 1FC9-koppelverzoek via **Apparaten → ventilator box → Stap voor stap instellen**.
4. Bekijk **Logboeken → Radio** en **Systeem**. De aanvraag moet eerst als ingepland gevolgd worden door de radio-uitkomst; let op een afwijzing wegens lage stackruimte en een nieuwe reset.
5. Als er opnieuw een panic is, bewaar de live USB-seriële uitvoer, de nieuwe diagnose-export en het bijpassende `firmware.elf`.
