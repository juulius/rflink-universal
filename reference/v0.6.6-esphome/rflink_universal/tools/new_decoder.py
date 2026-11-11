#!/usr/bin/env python3
"""Create one self-contained RFLink decoder pack from the beginner template."""
from __future__ import annotations

import argparse
import re
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DECODERS = ROOT / "decoders"
TEMPLATE = DECODERS / "_template"
DOC_DECODERS = ROOT.parent / "docs" / "source" / "rflink_universal" / "decoders"
ID_RE = re.compile(r"^[a-z][a-z0-9_]*$")


def class_name(pack_id: str) -> str:
    return "".join(part[:1].upper() + part[1:] for part in pack_id.split("_") if part)


def main() -> int:
    p = argparse.ArgumentParser(description="Create a beginner-friendly RFLink decoder pack")
    p.add_argument("pack_id", help="lowercase id, for example hood_remote_868")
    p.add_argument("display_name", help='human name, for example "Hood Remote 868"')
    args = p.parse_args()
    if not ID_RE.fullmatch(args.pack_id):
        p.error("pack_id must start with a lowercase letter and contain only a-z, 0-9 and _")
    dest = DECODERS / args.pack_id
    if dest.exists():
        p.error(f"decoder pack already exists: {dest}")
    docdest = DOC_DECODERS / args.pack_id
    if docdest.exists():
        p.error(f"documentation already exists: {docdest}")
    shutil.copytree(TEMPLATE, dest)
    shutil.copytree(DOC_DECODERS / '_template', docdest)
    replacements = {
        "__PACK_ID__": args.pack_id,
        "__DISPLAY_NAME__": args.display_name,
        "__CLASS_NAME__": class_name(args.pack_id),
    }
    for path in list(dest.rglob("*")) + list(docdest.rglob("*")):
        if not path.is_file():
            continue
        text = path.read_text(encoding="utf-8")
        for old, new in replacements.items():
            text = text.replace(old, new)
        path.write_text(text, encoding="utf-8")
    print(f"Created: {dest.relative_to(ROOT)} and {docdest.relative_to(ROOT.parent)}")
    print("Next: edit decoder.cpp, manifest.json and add captures/tests.")
    print("No central decoder list or constants file needs editing.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
