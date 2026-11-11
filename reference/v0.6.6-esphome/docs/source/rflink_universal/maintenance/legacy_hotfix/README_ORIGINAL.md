# HISTORISCH DOCUMENT — NIET DE ACTUELE INSTALLATIEHANDLEIDING
# Originele instructies hebben betrekking op de oude ZIP-indeling en losse hotfix.
# Gebruik nu uitsluitend START_HERE.md in de hoofdmap. De bestaande hotfix is al toegepast.

# RFLink ESPHome 2026.9 — webhandler linker hotfix

**Oorzaak:** ESPHome compileert de hoofdcomponent, maar de `RFLinkWebHandler`-vtable
wordt niet gevonden tijdens linken. Dit is een *buildfout*, niet een runtime reboot/crash.

**Wat is aangepast:** de inhoud van `rflink_web.cpp` is achteraan
`rflink_universal.cpp` geplaatst; het losse `.cpp` is bewust verwijderd.
De ESPHome component compileert de webhandler daarmee in dezelfde translation unit
als `RFLinkUniversal`. Overige protocol- en radiocode is vanaf de beschikbare
v0.3.2-RC3 bron overgenomen. De actuele RC4-bron zelf was niet bijgevoegd.

**Belangrijk voor je bestaande RC4:** als je daar nieuwere niet-gedeelde code in hebt,
gebruik dan `apply_hotfix.py` op de *bestaande* RC4-map in plaats van de complete
`rflink_universal/` map uit deze ZIP eroverheen te kopiëren. Zo behoud je alle
RC4-wijzigingen. Het script maakt eerst een back-up van beide betrokken bestanden.

## Wat moet je vervangen?

1. Bewaar een kopie van je huidige `/config/esphome/rflink-universal.yaml`
   en `/config/esphome/rflink_universal/` map.
2. Kopieer `apply_hotfix.py` naar `/config/esphome/` en voer uit:
   `python3 /config/esphome/apply_hotfix.py /config/esphome/rflink_universal`
   (via SSH/Terminal; gebruikt alleen Python standaardbibliotheek).
   Als je geen terminal gebruikt, kun je als alternatief de complete map uit
   deze ZIP kopiëren, maar **alleen als je bewust terug wilt naar de RC3-codebasis**.
3. Vervang de inhoud van je bestaande `rflink-universal.yaml` door het meegeleverde
   bestand met DEZELFDE naam. Verwijder het ESPHome-device niet en importeer niet
   een tweede device met dezelfde naam.
4. **Laat `secrets.yaml` staan**: de referenties naar je huidige vijf secret-namen
   zijn onveranderd. Je echte wachtwoorden/sleutels staan niet in de ZIP.
5. ESPHome: menu rechtsboven (`...`) -> **Clean Build Files** en daarna Install.
   De first-time build in je log kwam tot `[1131/1134]`; de patched bron moet
   nu wél de vtable bevatten. Een volledige ESPHome 2026.9 firmwarecompile is
   in deze omgeving niet uitvoerbaar; de laatste verificatie gebeurt in jouw ESPHome.

## Installeer je van nul af aan?

Leg `rflink-universal.yaml` naast de map `rflink_universal/`, onder
`/config/esphome/`. Alleen een YAML importeren maakt GEEN lokale custom
component- en decoderbestanden aan; die map moet apart worden meegestuurd.

## Veiligheid

De RC3-gebaseerde webinterface heeft nog geen geconfigureerde HTTP-authenticatie.
Zet deze daarom NIET open op internet of op niet-vertrouwde netwerken.
Een OTA-wachtwoord is voorlopig behouden omdat de bestaande firmwarestatus
onbekend is. Migreren naar OTA `encryption:` kan apart volgens ESPHome 2026.9.

## Niet getest / niet claimen

- Geen echte ESPHome 2026.9 firmwarecompile beschikbaar in deze omgeving.
- Geen hardware-RF of pairing getest.
- De actuele RC4-bron was niet meegestuurd: de full-copy is gebaseerd op RC3.
- Device persistence/HA MQTT discovery zijn nog niet werkend bewezen.
