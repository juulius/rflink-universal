# Protecting the main branch

RFLink Universal develops on short-lived branches. The default branch (`main`) is the reviewed integration branch, **not** proof that firmware is safe to flash.

## Set up GitHub rules (one-time administrator step)

Open [Repository Rulesets](https://github.com/juulius/rflink-universal/settings/rules), choose **New ruleset → New branch ruleset**, and use:

| Setting | Value |
| --- | --- |
| Ruleset name | `Protect main` |
| Enforcement status | **Active** |
| Target branches | Default branch (`main`) |
| Require a pull request before merging | **Enabled** |
| Required approving reviews | **0**, while only one maintainer contributes |
| Require status checks before merging | **Enabled** |
| Required checks | `validate`, `native-idf-build` |
| Require branches to be up to date before merging | **Enabled** (strict) |
| Require conversation resolution before merging | **Enabled** |
| Block force pushes | **Enabled** |
| Restrict deletions | **Enabled** |
| Allow bypass | None for routine development |

Do not set one required reviewer when you are the only maintainer: GitHub does not accept self-approval. If other contributors join, enable at least one independent review.

**Keep these options OFF initially:** merge queue, mandatory signed commits and required deployment environments. They need additional setup and could unintentionally prevent development.

The check names above come from [RFLink source checks](../.github/workflows/validate_source.yml). They must be selected **exactly** as GitHub presents them in the ruleset selector; if they are not listed, run the workflow on a pull request first.

GitHub Rulesets are available for **public GitHub Free** repositories. This file documents the policy; it does **not** activate protection by itself. Only a repository administrator can do that through GitHub Settings (or an authorized Rulesets API integration).

## Developer workflow

1. Make changes in a feature or fix branch, not directly in `main`.
2. Open a pull request and keep it in **Draft** until ready for review.
3. Require both tests and the official ESP-IDF build to pass before merging. Resolve open review conversations.
4. Check source attribution, security and documentation for any significant code import.
5. Merge after review (prefer **Squash and merge** for small change sets).
6. Keep firmware publication and physical device updates **separate** from merging code.

## Release safety

- A successful ESP-IDF build proves compilation, not RF behavior, pairing, OTA rollback, hardware safety, or feature completeness.
- **Never** automatically flash devices, publish a stable firmware release, or merge bypassing required checks.
- `v0.6.6 Radio Gaga` remains the working reference until native v0.7 passes real device tests.
- A future release workflow should require explicit approval and attestations of the exact board revision, source commit and test evidence.

## Troubleshooting

If GitHub shows **Waiting for required status checks**, verify that the check names match the job names in `validate_source.yml` and that the pull request has a recent check run.

If GitHub requires an approval from you on your own pull request, set required approvals to **0** until a second maintainer can review changes.

If the base branch changed, update the PR branch and rerun checks; this is normal with strict branch protection.
