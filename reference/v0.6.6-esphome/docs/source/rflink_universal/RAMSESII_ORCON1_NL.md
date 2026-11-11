# Orcon 15RF / RAMSES-II (RAMSES1)

Deze uitbreiding gebruikt dezelfde `manifest.json` + `decoder.cpp` structuur als de andere ingebouwde decoders. De protocolcodec staat in `codec.cpp/.h`. De Orcon MVS-15R / 15RF is een apart apparaatprofiel, omdat RAMSES-II ook door andere merken wordt gebruikt.

**Wat werkt in software:** twee echte pakketten (#653, #661) worden herkend, inclusief status Away/Auto, apparaat-ID 29:204810 en checksumcontrole. Het protocol is terug te vinden in Instellingen → Decoderbibliotheek. Het profiel moet je zelf bij het apparaat selecteren.

**Wat nog niet werkt:** volledige fysieke ontvangst (de radio staat nog in de Itho 63-byte-pakketmodus), ventilatie bedienen, zenden en koppelen. Er zijn geen radio-instellingen of GPIO's gewijzigd.

**Let op:** dit is een bijgewerkte broncodeversie, geen gecompileerd ESPHome-image. Eerst ESPHome Validate + Compile, daarna pas zelf OTA uitvoeren.

De bestaande decoders, Home Assistant-goedkeuring en TX-toestemmingen zijn ongemoeid gelaten.
