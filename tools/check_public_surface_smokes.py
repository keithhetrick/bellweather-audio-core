#!/usr/bin/env python3
# Copyright (c) 2026 Bellweather Studios.
# SPDX-License-Identifier: Apache-2.0

"""Verify public-surface smoke tests match the shipped module catalog."""

from __future__ import annotations

import argparse
import pathlib
import re
import sys


ROOT = pathlib.Path(__file__).resolve().parents[1]
MODULES_DOC = ROOT / "docs" / "MODULES.md"


def _stable_modules() -> set[str]:
    text = MODULES_DOC.read_text(encoding="utf-8")
    modules: set[str] = set()
    for match in re.finditer(r"^### `([^`]+)`\n(?P<body>.*?)(?=^### `|\Z)", text, flags=re.M | re.S):
        name = match.group(1)
        body = match.group("body")
        if re.search(r"^- Maturity: Stable/reusable\.", body, flags=re.M):
            modules.add(name)
    return modules


def _cmake_modules(cmake_list: pathlib.Path) -> set[str]:
    text = cmake_list.read_text(encoding="utf-8")
    match = re.search(r"set\(BWS_PUBLIC_SURFACE_MODULES(?P<body>.*?)\)", text, flags=re.S)
    if not match:
        raise SystemExit(f"ERROR: {cmake_list} does not define BWS_PUBLIC_SURFACE_MODULES")
    return set(re.findall(r"\b(bw_[A-Za-z0-9_]+)\b", match.group("body")))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--cmake-list", required=True, type=pathlib.Path)
    args = parser.parse_args()

    stable = _stable_modules()
    smokes = _cmake_modules(args.cmake_list)

    failures: list[str] = []
    missing = sorted(stable - smokes)
    extra = sorted(smokes - stable)
    if missing:
        failures.append("Stable/reusable modules without public-surface smoke tests: " + ", ".join(missing))
    if extra:
        failures.append("Smoke tests for modules not documented as Stable/reusable: " + ", ".join(extra))

    if failures:
        print("\n".join("ERROR: " + failure for failure in failures), file=sys.stderr)
        return 1

    print(f"PASS: public-surface smoke tests cover {len(stable)} Stable/reusable modules.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
