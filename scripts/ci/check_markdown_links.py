#!/usr/bin/env python3
"""Verify relative markdown links inside Documents/ resolve to existing paths.

Skips external (http://, https://, mailto:) and anchor-only (#section) links.
Reports broken links and exits non-zero if any are found.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path
from urllib.parse import unquote

REPO_ROOT = Path(__file__).resolve().parents[2]
DOCS_DIR = REPO_ROOT / "Documents"

LINK_RE = re.compile(r"!?\[[^\]]*\]\(([^)]+)\)")

errors: list[str] = []


def is_external(target: str) -> bool:
    return target.startswith(("http://", "https://", "mailto:", "ftp://", "//"))


def check_file(md: Path) -> None:
    text = md.read_text(encoding="utf-8", errors="replace")
    for match in LINK_RE.finditer(text):
        raw = match.group(1).strip()
        if not raw:
            continue
        # Strip title: [text](path "title")
        if " " in raw:
            raw = raw.split(" ", 1)[0]
        if is_external(raw):
            continue
        # Anchor-only on same file is fine; skip anchor portion.
        target = unquote(raw.split("#", 1)[0]).strip()
        if not target:
            continue
        if target.startswith("/"):
            resolved = REPO_ROOT / target.lstrip("/")
        else:
            resolved = (md.parent / target).resolve()
        if not resolved.exists():
            errors.append(
                f"{md.relative_to(REPO_ROOT)}: broken link -> {raw!r} "
                f"(resolved: {resolved})"
            )


def main() -> int:
    if not DOCS_DIR.is_dir():
        print("check_markdown_links.py: no Documents/ directory; skipping.")
        return 0

    md_files = sorted(DOCS_DIR.rglob("*.md"))
    if not md_files:
        print("check_markdown_links.py: no markdown files in Documents/; skipping.")
        return 0

    for md in md_files:
        check_file(md)

    if errors:
        print(
            f"check_markdown_links.py: {len(errors)} broken link(s):",
            file=sys.stderr,
        )
        for err in errors:
            print(f"  - {err}", file=sys.stderr)
        return 1

    print(f"check_markdown_links.py: OK — {len(md_files)} markdown file(s) checked.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
