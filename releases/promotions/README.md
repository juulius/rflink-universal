# Promoting firmware through a pull request

Do not edit published tags. Create a NEW JSON file for each promotion PR,
such as 2026-11-07-rc1.json, in this folder. Keep promotion PRs separate from
firmware changes. PR validation rejects changed or deleted old promotion files.

## Experimental -> Pre-release (RC)

Example file 2026-11-07-rc1.json:

    {
      "version": "0.7.0",
      "codename": "Radioactive",
      "channel": "pre-release",
      "promote_from": "firmware/v0.7.0-exp.3",
      "reason": "Bench-tested this source and ready to begin RC validation."
    }

When this PR is reviewed and merged, the published Experimental source
commit becomes the next immutable RC (rc.1, rc.2, etc.). No code is changed.

## Pre-release -> Stable

A Stable PR needs documented real-board test evidence and explicit approval.
The JSON must include version/codename/channel/promote_from/reason, plus:

    "stable_approval": "REAL_BOARD_TESTS_COMPLETE",
    "hardware_evidence": {
      "rf_43342_rx": "https://github.com/OWNER/REPO/issues/1",
      "rf_43392_rx": "https://github.com/OWNER/REPO/issues/1",
      "rf_86830_rx": "https://github.com/OWNER/REPO/issues/1",
      "tx_rx_restoration": "https://github.com/OWNER/REPO/issues/1",
      "wifi_usb": "https://github.com/OWNER/REPO/issues/1",
      "crash_persistence": "https://github.com/OWNER/REPO/issues/1",
      "ota_ab_rollback": "https://github.com/OWNER/REPO/issues/1",
      "recovery": "https://github.com/OWNER/REPO/issues/1"
    }

THESE LINKS ARE PLACEHOLDERS, NOT TEST EVIDENCE. Replace each URL with the
actual GitHub issue or PR documenting the test and its results.

The person merging a Stable promotion PR is explicitly approving the
documented hardware evidence. Protect main with required checks and human
reviewers before relying on this flow: automated validation verifies the
shape and presence of links, NOT actual physical RF behavior.

## After the merge

The existing source-check pipeline must pass on the merged main commit.
The automatic release workflow then recompiles the requested exact channel
with official ESP-IDF, verifies the built manifest and publishes a source-only
immutable GitHub Release. It never flashes a device or moves an old tag.
