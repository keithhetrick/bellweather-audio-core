#!/usr/bin/env python3
# Copyright (c) 2026 Bellweather Studios.
# SPDX-License-Identifier: Apache-2.0

"""Check that the public tree manifest accounts for every shipped path."""

from __future__ import annotations

import fnmatch
import pathlib
import re
import sys


ROOT = pathlib.Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "MANIFEST.md"
MODULES_DOC = ROOT / "docs" / "MODULES.md"
RULE_RE = re.compile(r"^- `([^`]+)` - .+$")
JUNK_NAMES = {".DS_Store", "__MACOSX", ".AppleDouble", "__pycache__"}
JUNK_PATTERNS = ("._*", "*.orig", "*.bak", "*.rej", "*~", "*.swp", "*.pyc")
BUILD_DIR_PATTERNS = ("build", "build-*")


def _rel(path: pathlib.Path) -> str:
    return path.relative_to(ROOT).as_posix()


def _ignored_path(rel: str) -> bool:
    if rel == ".git" or rel.startswith(".git/"):
        return True
    top = pathlib.PurePosixPath(rel).parts[0]
    return any(fnmatch.fnmatchcase(top, pat) for pat in BUILD_DIR_PATTERNS)


def _coverage_patterns() -> list[str]:
    if not MANIFEST.exists():
        raise SystemExit(f"ERROR: missing {MANIFEST.relative_to(ROOT)}")

    patterns: list[str] = []
    for line in MANIFEST.read_text(encoding="utf-8").splitlines():
        match = RULE_RE.match(line)
        if match:
            patterns.append(match.group(1).rstrip("/"))

    if not patterns:
        raise SystemExit("ERROR: MANIFEST.md contains no coverage rules")
    return patterns


def _covered(path: str, patterns: list[str]) -> bool:
    for pattern in patterns:
        if pattern.endswith("/**"):
            base = pattern[:-3]
            if path == base or path.startswith(base + "/"):
                return True
        elif fnmatch.fnmatchcase(path, pattern):
            return True
    return False


def _all_paths() -> list[str]:
    paths: list[str] = []
    for child in ROOT.rglob("*"):
        rel = _rel(child)
        if _ignored_path(rel):
            continue
        paths.append(rel)
    return sorted(paths)


def _empty_dirs() -> list[str]:
    empty: list[str] = []
    for path in ROOT.rglob("*"):
        rel = _rel(path)
        if _ignored_path(rel):
            continue
        if path.is_dir() and not any(path.iterdir()):
            empty.append(rel)
    return sorted(empty)


def _junk_paths(paths: list[str]) -> list[str]:
    bad: list[str] = []
    for path in paths:
        name = pathlib.PurePosixPath(path).name
        if name in JUNK_NAMES or any(fnmatch.fnmatchcase(name, pat) for pat in JUNK_PATTERNS):
            bad.append(path)
    return bad


def _module_doc_gaps() -> list[str]:
    if not MODULES_DOC.exists():
        return ["docs/MODULES.md"]

    text = MODULES_DOC.read_text(encoding="utf-8")
    documented = set(re.findall(r"^### `([^`]+)`", text, flags=re.MULTILINE))
    modules = {
        path.name
        for path in (ROOT / "modules").glob("bw_*")
        if path.is_dir()
    }
    return sorted(modules - documented)


def main() -> int:
    patterns = _coverage_patterns()
    paths = _all_paths()

    failures: list[str] = []

    uncovered = [path for path in paths if not _covered(path, patterns)]
    if uncovered:
        failures.append(
            "Uncovered paths:\n  " + "\n  ".join(uncovered[:200])
            + ("\n  ..." if len(uncovered) > 200 else "")
        )

    empty = _empty_dirs()
    if empty:
        failures.append("Empty directories:\n  " + "\n  ".join(empty))

    junk = _junk_paths(paths)
    if junk:
        failures.append("Junk paths:\n  " + "\n  ".join(junk))

    module_gaps = _module_doc_gaps()
    if module_gaps:
        failures.append("Modules missing docs/MODULES.md entries:\n  " + "\n  ".join(module_gaps))

    if failures:
        print("\n\n".join("ERROR: " + failure for failure in failures), file=sys.stderr)
        return 1

    print(f"PASS: MANIFEST.md covers {len(paths)} shipped paths.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
