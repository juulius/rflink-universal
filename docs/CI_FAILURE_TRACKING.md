# Automatic build failure reporting

RFLink uses two separate GitHub Actions workflows:

1. **RFLink source checks** runs software tests and, when firmware source is present, compiles with the official ESP-IDF toolchain.
2. **RFLink CI failure tracker** observes completed source checks and updates GitHub Issues.

## What happens automatically?

- **Failed build:** a GitHub Issue is opened with the failed job, step, up to three filtered diagnostic lines when available, and a link to the complete GitHub Actions logs.
- **Further failure on the same PR or branch:** the existing Issue is updated rather than creating a duplicate.
- **Successful later build:** the open Issue for that PR or branch is closed with a link to the passing run.
- **Cancelled runs:** no issue is opened or closed.
- **Older results:** a late, older workflow result does not overwrite a newer result.

[See the automatically reported issues](https://github.com/juulius/rflink-universal/issues).

## Privacy and security

The tracker executes from the repository's trusted default branch using the GitHub Actions `workflow_run` event. It does **not** check out or run the code in an untrusted pull request. Its temporary token can read Actions metadata and write Issues, but cannot modify firmware source.

Only short, filtered error excerpts are placed in public Issues, not complete logs. Do not put secrets in compiler logs or source files; filters are a precaution, not a guarantee that every possible sensitive value can be recognized.

## No AI subscription or autonomous code changes required

The tracker uses GitHub Actions and `actions/github-script`, not a paid Copilot agent or OpenAI API. It **does not fix compiler errors itself**. A developer can inspect the created Issue, correct the development branch, and let CI run again. No automatic merge, firmware Release, USB flash or OTA installation occurs.

## If the tracker cannot create Issues

Check **Repository Settings → Actions → General → Workflow permissions**. The tracker explicitly requests `issues: write` for its GitHub token, and any additional organization-level restrictions may need review.

The actual GitHub Actions executions are the integration test of this automation. The current firmware's successful ESP-IDF compilation is separate from hardware validation.
