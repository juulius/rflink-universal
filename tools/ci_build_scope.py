#!/usr/bin/env python3
"""Decide whether a PR needs the expensive native ESP-IDF build.

Pull request checks run on GitHub's synthetic merge commit. Only a small,
explicit allow-list of non-firmware changes may skip the native build.
Post-merge pushes run fast checks; any firmware release still compiles its
pinned source using the final release identity before publication.
"""
from __future__ import annotations

import os
import subprocess
from pathlib import Path

SAFE_FILES = {
    ".gitignore", "AGENTS.md", "README.md", "repository.yaml",
    ".github/workflows/ci_failure_tracker.yml",
    ".github/workflows/prepare_builder_image.yml",
    ".github/workflows/refresh_firmware_bundle.yml",
    ".github/workflows/stage_source_import.yml",
}
SAFE_PREFIXES = (
    "docs/", "hardware/", "experiments/", "tests_repo/", "tests_previous/",
)
# These files participate in the real-image provisioning / USB packaging gate.
NATIVE_BUILDER_FILES = {
    "rflink_builder/default_source.zip",
    "rflink_builder/provisioning.py",
    "rflink_builder/browser_flash.py",
    "rflink_builder/toolchain.py",
}


def safe_without_native_build(path: str) -> bool:
    if not path or path.startswith("/") or ".." in Path(path).parts:
        return False
    if path in SAFE_FILES or path.startswith(SAFE_PREFIXES):
        return True
    if path.startswith("rflink_builder/"):
        return path not in NATIVE_BUILDER_FILES
    # Promotion PRs only reference already built source; the release workflow
    # recompiles that exact source with the final promotion identity.
    if path.startswith("releases/promotions/"):
        return path.endswith(".json") and path.count("/") == 2
    return False


def needs_native_build(changed_paths: list[str]) -> bool:
    # Unexpected empty/malformed diffs fail closed.
    return not changed_paths or any(
        not safe_without_native_build(path) for path in changed_paths
    )


def pr_merge_changed_paths() -> list[str]:
    parents = subprocess.check_output(
        ["git", "rev-list", "--parents", "-n", "1", "HEAD"], text=True
    ).split()
    if len(parents) != 3:
        raise RuntimeError("Expected a GitHub pull-request merge commit with two parents")
    # --no-renames checks both sides of a rename. NUL separation covers unusual
    # path names and avoids interpreting file names as shell code.
    raw = subprocess.check_output(
        ["git", "diff", "--no-renames", "--name-only", "-z", "HEAD^1", "HEAD", "--"]
    )
    return [name for name in raw.decode("utf-8").split("\0") if name]


def main() -> int:
    event = os.environ.get("GITHUB_EVENT_NAME")
    if event == "pull_request":
        files = pr_merge_changed_paths()
        required = needs_native_build(files)
        print(f"PR merge changes ({len(files)}): {', '.join(files[:25])}")
    elif event == "push" and os.environ.get("GITHUB_REF") == "refs/heads/main":
        # No build here: the auto-release build uses the exact merged commit and
        # final version. A failed release build cannot publish a tag.
        required = False
        print("Main: defer native compilation to the release build")
    else:
        # workflow_dispatch and unknown events keep the strict full build.
        required = True
        print(f"{event or 'unknown'}: full native build required")
    output = os.environ["GITHUB_OUTPUT"]
    with open(output, "a", encoding="utf-8") as f:
        f.write(f"idf_build_required={str(required).lower()}\n")
    print("ESP-IDF native build:", "required" if required else "skipped")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
