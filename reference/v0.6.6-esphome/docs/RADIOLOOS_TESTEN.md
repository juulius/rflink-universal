# RFLink Universal zonder CC1101 modules

Deze versie behandelt drie ontbrekende CC1101's als een perifere storing, niet als een reden om web/API/OTA uit te schakelen.

- Status: een aangesloten, door ESPHome geïnitialiseerde CC1101 wordt `RX`; een mislukte CC1101-initialisatie `Niet aangesloten`.
- TX-acties voor mislukte CC1101's worden overgeslagen met een waarschuwing.
- Ontbrekende radio's worden niet gebruikt voor RFLink-receive en de GDO2-ontvangeringangen hebben een interne pulldown tegen zwevende signalen.
- Na het later aansluiten van een radiomodule moet de ESP32 herstart worden; een door ESPHome als failed gemarkeerde component herstelt niet automatisch.

**Belangrijk:** deze code is getest met hosttests, YAML-controles en een C++ compile smoke-test, maar niet op echte ESP32-S3-hardware. Dit bewijst niet dat het gemelde probleem met HTTP én OTA is opgelost. Houd de originele installatie en seriële herstelmogelijkheid beschikbaar.

## Verificatie op het apparaat

1. Gebruik USB om de bootlog te zien vanaf RESET. Controleer `Failed to verify CC1101` en vervolgens `setup() finished successfully`, IP-adres, OTA- en webservermeldingen.
2. Probeer `http://<actueel-ip>/rflink` (geen HTTPS) en controleer of alle drie de radio's `Niet aangesloten` tonen.
3. Controleer de ESPHome API en OTA afzonderlijk op hetzelfde actuele IP. Een DHCP-lease alleen bewijst niet dat de firmware nog draait.
4. Als alle netwerkdiensten onbereikbaar blijven, leg de eerste reset/crash/stacktrace of laatste setupregels van de seriële USB-log vast. Dan kunnen we de daadwerkelijke bootfout isoleren.

Let op: wanneer je bij import `rflink-universal.yaml` behoudt, krijgt de oude configuratie niet de nieuwe radio-controle en pulldown-instellingen. Gebruik voor deze overgang de meegeleverde veilige YAML-updater, bekijk eventuele mergeconflicten en laat `MAAK_RELEASE.bat` pas een bundle maken wanneer de updater READY meldt. Overschrijf nooit blind eigen GPIO-/Itho-instellingen.
