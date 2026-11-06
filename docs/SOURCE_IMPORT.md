# Import the RFLink source in one upload

This is a **one-time** setup procedure for the native Radioactive v0.7 firmware and RFLink Universal Builder. You do **not** need to upload 150 files individually.

> **Important:** These are development sources, not verified firmware binaries. Do not flash them over a working Radio Gaga v0.6.6 device. Review the [publication checklist](PUBLICATION_CHECKLIST.md) and the protocol attribution notice before making imported code public.

## Import

1. Download the prepared `rflink-source-import.zip` file from the RFLink development conversation. **Keep its filename exactly as shown. Do not extract it.**
2. In [the GitHub repository](https://github.com/juulius/rflink-universal), select **Add file → Upload files** (or use [the upload page](https://github.com/juulius/rflink-universal/upload/main)).
3. Drag the single ZIP file into the upload area and **commit to `main`**. This makes the ZIP publicly visible. Never use this flow for secrets or personal RF captures.
4. Open [GitHub Actions](https://github.com/juulius/rflink-universal/actions) and find **Stage RFLink source import**. It validates file paths and creates an `import/radioactive-…` branch. It does not flash or release firmware.
5. Review the new branch's changes, attribution and tests, then open a pull request to `main`. The [source checks](../.github/workflows/validate_source.yml) run the Python/JavaScript tests and attempt a **real ESP-IDF 5.5.6 compilation**. Fix any failures before considering the code ready for use.
6. Merge the reviewed pull request when appropriate. The branch removes the temporary ZIP from the current repository files and introduces the source folders. Previous Git history still retains uploaded content, so **never upload a ZIP containing private information**.

If the import workflow cannot create a branch, check **Repository Settings → Actions → General → Workflow permissions** and ensure the workflow has permission to write repository contents. Do not grant broader permissions than needed.

## After the source has been merged

The repository will contain `rflink_builder/config.yaml`, `rflink_builder/Dockerfile`, `firmware/`, tests and `repository.yaml`. Then Home Assistant OS can load the app repository through **Settings → Apps → Install app → ⋮ → Repositories** using:

`https://github.com/juulius/rflink-universal`

Do not interpret app installation as proof that a firmware build or OTA update is safe. First build on a test environment and review the logs. Keep local GPIO/SPI overrides, passwords and MQTT details in the Home Assistant Builder's persistent storage, not in GitHub.

## Release workflow

Builder updates use the version in `rflink_builder/config.yaml`. Firmware versions are tracked separately with GitHub Releases (for example `firmware/v0.7.0`). The Builder's default selector shows five previous stable versions and **All releases** shows the full history. A new source commit is not automatically a published or hardware-validated firmware release.
