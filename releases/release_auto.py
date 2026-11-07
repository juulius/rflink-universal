#!/usr/bin/env python3
"""PR-gated RFLink release planning. No tags are moved or releases overwritten.

A PR check validates source changes or a one-shot promotion request.
After the existing full source checks pass on a merged main commit, a separate
workflow resolves the exact immutable source SHA and next channel sequence.
"""
from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CONFIG = ROOT / "releases" / "RELEASE_AUTOMATION.json"
PROMOTION_PREFIX = "releases/promotions/"
SHA = re.compile(r"^[a-f0-9]{40}$")
VERSION = re.compile(r"^0[.]7[.]0$")
NAME = re.compile(r"^[a-zA-Z0-9._-]+[.]json$")
EVIDENCE = (
    "rf_43342_rx", "rf_43392_rx", "rf_86830_rx", "tx_rx_restoration",
    "wifi_usb", "crash_persistence", "ota_ab_rollback", "recovery",
)
# A Builder-only or documentation PR must not create identical firmware tags.
FIRMWARE_PREFIXES = (
    "firmware/main/", "firmware/components/", "firmware/tools/",
    "firmware/boards/", "firmware/tests/",
)
FIRMWARE_FILES = {
    "firmware/CMakeLists.txt", "firmware/sdkconfig.defaults",
    "firmware/partitions.csv", "firmware/SOURCE_MANIFEST.json",
    "releases/RELEASE_AUTOMATION.json",
}


def command(*args: str, check: bool = True) -> str:
    p = subprocess.run(args, cwd=ROOT, text=True, capture_output=True)
    if check and p.returncode:
        raise ValueError(f"{' '.join(args[:3])} failed: {p.stderr.strip()[:500]}")
    return p.stdout


def config() -> dict:
    data = json.loads(CONFIG.read_text(encoding="utf-8"))
    if set(data) != {"version", "codename", "automatic_experimental"}:
        raise ValueError("Release config has unexpected fields")
    if not VERSION.fullmatch(data["version"]) or data["codename"] != "Radioactive":
        raise ValueError("Only the native v0.7.0 Radioactive build is currently supported")
    if type(data["automatic_experimental"]) is not bool:
        raise ValueError("automatic_experimental must be a boolean")
    return data


def changes(base: str, head: str) -> list[tuple[str, str]]:
    # For a merged PR use GitHub's complete file list, which correctly covers
    # merge, squash and rebase strategies (HEAD^ only sees a rebase's last commit).
    filelist = os.environ.get("RFLINK_PR_FILES_JSON")
    if filelist:
        items = json.loads(Path(filelist).read_text(encoding="utf-8"))
        if not isinstance(items, list) or not 1 <= len(items) <= 1000:
            raise ValueError("Merged PR changed-file list is missing or too large")
        rows = []
        statuses = {"added": "A", "modified": "M", "removed": "D"}
        for item in items:
            if not isinstance(item, dict) or not isinstance(item.get("filename"), str):
                raise ValueError("Malformed merged PR file metadata")
            path = item["filename"]
            status = item.get("status")
            if status == "renamed":
                original = item.get("previous_filename")
                if not isinstance(original, str):
                    raise ValueError("Renamed PR file must have original path")
                rows.extend([("D", original), ("A", path)])
            elif status in statuses:
                rows.append((statuses[status], path))
            else:
                raise ValueError("Unsupported merged PR file status")
        return rows
    lines = command("git", "diff", "--name-status", "--no-renames", base, head, "--")
    result = []
    for line in lines.splitlines():
        status, sep, path = line.partition("\t")
        if sep and status in {"A", "M", "D"}:
            result.append((status, path))
        else:
            raise ValueError("Unsupported or malformed Git diff status")
    return result


def firmware_changed(rows: list[tuple[str, str]]) -> bool:
    return any(path in FIRMWARE_FILES or path.startswith(FIRMWARE_PREFIXES)
               for _, path in rows)


def parse_promotion(path: str, expected_version: str) -> dict:
    filename = path.removeprefix(PROMOTION_PREFIX)
    if not NAME.fullmatch(filename) or "/" in filename:
        raise ValueError("Promotion request must have a simple .json filename")
    data = json.loads((ROOT / path).read_text(encoding="utf-8"))
    required = {"version", "codename", "channel", "promote_from", "reason"}
    if set(data) not in (required, required | {"hardware_evidence", "stable_approval"}):
        raise ValueError("Unexpected promotion fields; stable requires evidence + approval")
    if data.get("version") != expected_version or data.get("codename") != "Radioactive":
        raise ValueError("Promotion version and codename do not match release family")
    channel = data.get("channel")
    if channel not in {"pre-release", "stable"}:
        raise ValueError("Promotion must target pre-release or stable")
    if not isinstance(data.get("reason"), str) or not 12 <= len(data["reason"]) <= 2000:
        raise ValueError("Promotion needs a meaningful 12-2000 character reason")
    kind = "exp" if channel == "pre-release" else "rc"
    if not isinstance(data.get("promote_from"), str) or not re.fullmatch(
        rf"firmware/v0[.]7[.]0-{kind}[.][1-9][0-9]*", data["promote_from"]
    ):
        raise ValueError("Promotion must reference an existing immutable prior-channel tag")
    if channel == "pre-release" and set(data) != required:
        raise ValueError("RC promotion must not contain Stable-only assertions")
    if channel == "stable":
        if set(data) != required | {"hardware_evidence", "stable_approval"}:
            raise ValueError("Stable requires evidence and explicit approval")
        if data.get("stable_approval") != "REAL_BOARD_TESTS_COMPLETE":
            raise ValueError("Stable requires REAL_BOARD_TESTS_COMPLETE approval phrase")
        evidence = data.get("hardware_evidence")
        if not isinstance(evidence, dict) or set(evidence) != set(EVIDENCE):
            raise ValueError("Stable requires evidence for each actual hardware test")
        for test, link in evidence.items():
            if not isinstance(link, str) or not re.fullmatch(
                r"https://github[.]com/[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+/(?:issues|pull)/[1-9][0-9]*(?:#[-A-Za-z0-9._]+)?",
                link,
            ):
                raise ValueError(f"Stable evidence {test} must link to a documented GitHub issue/PR")
    return data


def plan(base: str, head: str) -> dict:
    setting = config()
    rows = changes(base, head)
    # Documentation in promotions/ is not itself a promotion request.
    requests = [path for status, path in rows
                if path.startswith(PROMOTION_PREFIX) and path.endswith(".json") and status == "A"]
    touched = [path for status, path in rows
               if path.startswith(PROMOTION_PREFIX) and path.endswith(".json") and status != "A"]
    if touched:
        raise ValueError("Published promotion requests are immutable: create a new request file")
    if len(requests) > 1:
        raise ValueError("Only one promotion may be requested per PR")
    if requests:
        if firmware_changed(rows):
            raise ValueError("Promotion PRs must not modify firmware: promote a tested source tag")
        other = [path for _, path in rows if path != requests[0] and not path.startswith("docs/")]
        if other:
            raise ValueError("Promotion PR must contain only one request and optional documentation")
        promotion = parse_promotion(requests[0], setting["version"])
        return {"action": "promote", "channel": promotion["channel"],
                "from_tag": promotion["promote_from"], "reason": promotion["reason"],
                "request": requests[0], "config": setting}
    if setting["automatic_experimental"] and firmware_changed(rows):
        return {"action": "experimental", "channel": "experimental", "config": setting}
    return {"action": "skip", "channel": "", "config": setting}


def published_tag(tag: str) -> str:
    """An existing git tag alone is insufficient: require a public GitHub Release."""
    release = json.loads(command("gh", "release", "view", tag, "--json", "tagName,isPrerelease"))
    if release.get("tagName") != tag or release.get("isPrerelease") is not True:
        raise ValueError("Promotion source must be a published prior-channel prerelease")
    sha = command("git", "rev-parse", "--verify", f"refs/tags/{tag}^{{commit}}").strip()
    if not SHA.fullmatch(sha):
        raise ValueError("Promotion tag does not point to a commit")
    return sha


def published_for_merge(merge_sha: str) -> str | None:
    """Avoid creating a second version when a passing source-check run is re-run."""
    repo = os.environ.get("GITHUB_REPOSITORY", "")
    if not re.fullmatch(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+", repo):
        return None  # Only the actual GitHub publication job has this context.
    pages = json.loads(command(
        "gh", "api", "--paginate", "--slurp", f"repos/{repo}/releases?per_page=100"
    ))
    if not isinstance(pages, list) or not all(isinstance(page, list) for page in pages):
        raise ValueError("Unable to verify existing GitHub Releases before publishing")
    needle = f"Merge commit: {merge_sha}"
    for page in pages:
        for item in page:
            if (isinstance(item, dict) and not item.get("draft") and
                    needle in str(item.get("body") or "") and
                    re.fullmatch(r"firmware/v0[.]7[.]0(?:-exp[.][1-9][0-9]*|-rc[.][1-9][0-9]*)?",
                                 str(item.get("tag_name") or ""))):
                return item["tag_name"]
    return None


def stable_exists(version: str) -> bool:
    return bool(command("git", "ls-remote", "--tags", "origin",
                        f"refs/tags/firmware/v{version}").strip())


def next_sequence(version: str, prefix: str) -> int:
    # Read *remote tags*, including any tag without a completed GitHub Release.
    # A number is never reused, even following a failed/crashed prior publication.
    pattern = f"refs/tags/firmware/v{version}-{prefix}.*"
    output = command("git", "ls-remote", "--tags", "origin", pattern)
    matcher = re.compile(rf"refs/tags/firmware/v{re.escape(version)}-{prefix}[.]([1-9][0-9]*)$")
    seq = [int(m.group(1)) for line in output.splitlines()
           if (m := matcher.search(line))]
    return max(seq, default=0) + 1


def resolve(plan_data: dict, merged_sha: str) -> dict:
    if not SHA.fullmatch(merged_sha):
        raise ValueError("Merged commit must be an exact SHA-1 commit ID")
    setting = plan_data["config"]
    version = setting["version"]
    channel = plan_data["channel"]
    if channel == "experimental":
        if stable_exists(version):
            raise ValueError("This version is already Stable; review a new native firmware version before more Experimental builds")
        seq = next_sequence(version, "exp")
        tag = f"firmware/v{version}-exp.{seq}"
        source_sha = merged_sha
        title = f"RFLink v{version} Radioactive · Experimental · build {seq}"
    elif channel == "pre-release":
        seq = next_sequence(version, "rc")
        tag = f"firmware/v{version}-rc.{seq}"
        source_sha = published_tag(plan_data["from_tag"])
        title = f"RFLink v{version} Radioactive · Pre-release · RC {seq}"
    elif channel == "stable":
        seq = 0
        tag = f"firmware/v{version}"
        source_sha = published_tag(plan_data["from_tag"])
        title = f"RFLink v{version} Radioactive · Stable"
    else:
        raise ValueError("Unsupported release action")
    if command("git", "ls-remote", "--tags", "origin", f"refs/tags/{tag}").strip():
        raise ValueError(f"Immutable release tag already exists: {tag}")
    return {**plan_data, "version": version, "tag": tag, "sequence": seq,
            "source_sha": source_sha, "merge_sha": merged_sha, "title": title}


def cli() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("mode", choices=("pr-check", "resolve"))
    p.add_argument("--base", required=True)
    p.add_argument("--head", required=True)
    p.add_argument("--output")
    args = p.parse_args()
    data = plan(args.base, args.head)
    if args.mode == "pr-check":
        if data["action"] == "experimental":
            if stable_exists(data["config"]["version"]):
                raise ValueError("This version is already Stable: start a new version family before merging firmware changes")
            print("Release check: Experimental will auto-publish after successful main CI; next number chosen at publish time.")
        elif data["action"] == "promote":
            published_tag(data["from_tag"])
            print(f"Release check: requested {data['channel']} promotion from {data['from_tag']} (source pinned; no retag).")
        else:
            print("Release check: no firmware changes; no new firmware version required.")
        return 0
    if data["action"] == "skip":
        report = {"action": "skip", "channel": "", "tag": "", "source_sha": "", "title": "", "from_tag": ""}
    else:
        # Git accepts symbolic HEAD for diffing; publication requires the
        # resolved immutable 40-character commit ID, never the text "HEAD".
        head_sha = command("git", "rev-parse", "--verify", args.head).strip()
        existing = published_for_merge(head_sha)
        if existing:
            print(f"Merged PR already published as {existing}; no duplicate release.")
            report = {"action": "skip", "channel": "", "tag": "", "source_sha": "",
                      "title": "", "from_tag": ""}
        else:
            report = resolve(data, head_sha)
            print(f"Planned {report['tag']} from {report['source_sha'][:12]} (not yet published)")
    if args.output:
        with open(args.output, "a", encoding="utf-8") as f:
            for key in ("action", "channel", "tag", "source_sha", "title", "from_tag"):
                f.write(f"{key}={report.get(key, '')}\n")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(cli())
    except (ValueError, OSError, KeyError, json.JSONDecodeError) as err:
        print(f"FAIL release planning: {err}", file=sys.stderr)
        raise SystemExit(1)
