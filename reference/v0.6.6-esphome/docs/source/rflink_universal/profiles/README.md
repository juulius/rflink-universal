# Built-in device profiles

These files are source data for RFLink Universal's read-only device-profile catalog.
They describe a **known product/model family on top of a decoder protocol**; they do
not replace the protocol decoder itself.

Rules:

- one JSON file per stable profile ID;
- every profile must point at an enabled decoder protocol;
- expected actions must exist in that decoder capability;
- source URLs are evidence for model/protocol association, not a claim that every
  hardware revision was physically tested on RFLink Universal;
- `hardware_validation: pending_on_rflink_universal` must remain until the exact
  model has been exercised on the new board and captured/replayed where relevant;
- user-created profiles in NVS remain separate and may override a built-in profile
  with the same stable ID.

Run `python tools/generate_profiles.py` after editing. The generator validates the
catalog and writes `components/rflink_universal/profile_catalog_generated.h`.
