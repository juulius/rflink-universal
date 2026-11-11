# Beginner decoder template

Use `python tools/new_decoder.py my_protocol "My Protocol"` instead of editing this folder directly.

## What you normally change

1. `match()` — describe how RFLink can recognize the protocol safely.
2. `decode()` — extract the stable device identity, action and values.
3. `manifest.json` — describe frequency, mode, pairing/TX and identity rules.
4. `tests/` — save captures that prove your decoder keeps working.

You do **not** edit a central constants file, decoder list, Home Assistant code or web UI.
