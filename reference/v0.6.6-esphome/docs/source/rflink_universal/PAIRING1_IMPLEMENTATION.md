# Pairing1 - veilige universele installatie-wizard

Ingebouwde en eigen profielen hebben dezelfde pair_method/controls/setup_steps API. Oude gebruikersprofielen in NVS snapshot v1 blijven leesbaar; nieuwe snapshots hebben v2 (2 onafhankelijke copies). Sensorprofielen hebben geen koppelactie; Somfy/Itho behouden bestaande expliciete zendpaden; RAMSES/Orcon TX blijft geblokkeerd totdat de 1FC9-handshake en CC1101 fysiek gevalideerd zijn. Een succesvol uitgezonden pakket is geen bewijs dat een toestel gekoppeld of bediend is.
