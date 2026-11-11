# TXROUTER2 + DEVICEUI2 – 27 oktober 2026

## Gerealiseerd
- Runtime kiest voorkeurs-/fallbacktransport alleen uit goedgekeurde decoder-manifesten en werkelijke adapterbeschikbaarheid.
- TX-plan heeft een door de ingebouwde encoder vastgelegde protocolidentiteit; radio, profiel, frametype en permissie worden opnieuw gecontroleerd vóór zendactie.
- Generieke `SerialTxAdapter`-interface met fysieke lease-, herstel- en controleverantwoordelijkheid. De core ondersteunt nu RMT RAW, SPI-FIFO en GDO-serieel langs dezelfde transportdispatcher. Er is nog **geen geregistreerde fysieke GDO-adapter**.
- Bestaande Itho-koppelroute gebruikt hetzelfde vertrouwde TX-profiel en behoudt SPI-FIFO.
- Op de apparatenpagina blijven geopende detailpanelen open na automatisch verversen; compacter responsive raster voor apparaat, bediening en profiel.
- Geen nieuwe GPIO-, CC1101- of opslagmigratie. 6 MHz SPI-instelling van bestaande project-YAML blijft intact bij gebruik van de patch.

## Teststatus en beperkingen
- Geslaagd: C++ host-syntax, generieke 100-profiel-routerbenchmark, GDO-adaptermock, ESPHome-padresolutie, TX3-RC/Dooya/MQTT-regressie, browser 1300px/390px detailpanelen, PAIRING1-browser en web_embed-controle.
- Nog niet afgerond: volledige ESPHome/ESP-IDF-build, complete Somfy-suite, fysieke GDO0-transmissie, herstel van Itho-RX en werkende Orcon-koppeling/bediening.
- Orcon-TX blijft niet beschikbaar: het manifest heeft `tx: none`, `enabled: false`, en er bestaat geen fysiek gevalideerde zenderadapter. De knoppen blijven terecht niet klikbaar.
- Dit is een veilige ontwikkelrelease; niet presenteren als een werkende Orcon-zender.
