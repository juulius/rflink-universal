"""Source-to-flash identity checks shared by every RFLink install path.

A version string or board hash alone does not identify source code. Published
firmware must retain the exact immutable Git commit selected for that device.
No network access or device credentials are needed for these checks.
"""
from __future__ import annotations
import re

SHA40 = re.compile(r"[0-9a-f]{40}\Z")
SHA256 = re.compile(r"[0-9a-f]{64}\Z")


def build_identity(manifest: dict, metadata: dict | None = None) -> dict:
    """Public, non-secret provenance for build and installation UI."""
    metadata = metadata or {}
    files = manifest.get("files") or {}
    app = files.get("app_ota") or {}
    release_tag = manifest.get("release_tag")
    return {
        "version": manifest.get("version") or "",
        "release_tag": release_tag,
        "source_sha": manifest.get("source_commit_sha") or metadata.get("source_sha"),
        "source_fingerprint": manifest.get("source_fingerprint") or "",
        "image_sha256": app.get("sha256") or "",
        "board_profile_hash": manifest.get("board_profile_hash") or "",
        "source_kind": "github_release" if release_tag else "local_development",
    }


def require_selected_build(manifest: dict, selection, metadata: dict | None = None) -> dict:
    """Fail closed if a build is not from the exact selected release/commit.

    Existing local development packages remain usable. Older published
    packages without a traceable source SHA must be rebuilt, not silently
    installed by matching only a version label or board-profile hash.
    """
    if not isinstance(manifest, dict):
        raise RuntimeError("Firmware manifest is missing")
    expected_tag = selection.get("tag") if isinstance(selection, dict) else None
    expected_sha = selection.get("sha") if isinstance(selection, dict) else None
    tag = manifest.get("release_tag")
    if tag != expected_tag:
        raise RuntimeError("Firmware release changed. Build or select the chosen version again.")
    if metadata is not None and metadata.get("release_tag") != tag:
        raise RuntimeError("Stored build and firmware manifest identify different releases.")
    if expected_tag:
        if (not isinstance(expected_sha, str) or not SHA40.fullmatch(expected_sha)):
            raise RuntimeError("Selected firmware source commit is invalid.")
        actual = manifest.get("source_commit_sha")
        # For pre-provenance packages, older metadata is still not enough:
        # the source SHA must be carried by the installation manifest itself.
        if actual != expected_sha:
            raise RuntimeError("Firmware source commit is missing or differs. Rebuild this release.")
        if metadata is not None and metadata.get("source_sha") != actual:
            raise RuntimeError("Stored build source differs from the firmware manifest.")
    elif manifest.get("source_commit_sha"):
        raise RuntimeError("A published source is labelled as local development firmware.")
    identity = build_identity(manifest, metadata)
    if expected_tag and (not identity["version"] or
                         not SHA256.fullmatch(identity["image_sha256"])):
        raise RuntimeError("Published firmware build identity is incomplete. Rebuild.")
    return identity
