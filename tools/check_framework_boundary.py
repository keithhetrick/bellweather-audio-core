#!/usr/bin/env python3
# Copyright (c) 2026 Bellweather Studios.
# SPDX-License-Identifier: Apache-2.0

"""Verify that reusable core headers do not include or name JUCE."""

from __future__ import annotations

import argparse
import pathlib
import re
import sys


PATTERN = re.compile(
    r"juce::"
    r"|#\s*include\s*[<\"].*juce"
    r"|\busing\s+namespace\s+juce\b"
    r"|\bnamespace\s+juce\s*\{",
    re.IGNORECASE,
)
REUSABLE_MODULES = (
    "bw_core",
    "bw_audio_types",
    "bw_state_types",
    "bw_ports",
    "bw_rt",
    "bw_fft_adapters",
    "bw_dsp_core",
    "bw_dsp_metering",
    "bw_dsp_processors",
)


def strip_line_comment(line: str) -> str:
    return line.split("//", 1)[0]


def is_comment_only(line: str) -> bool:
    stripped = line.lstrip()
    return stripped.startswith("//") or stripped.startswith("*")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--root",
        default=pathlib.Path(__file__).resolve().parents[1],
        type=pathlib.Path,
        help="repository root to scan",
    )
    args = parser.parse_args()

    root = args.root.resolve()
    modules_dir = root / "modules"
    if not modules_dir.is_dir():
        print(f"FAIL: modules directory not found: {modules_dir}", file=sys.stderr)
        return 1

    findings: list[str] = []
    for module in REUSABLE_MODULES:
        module_dir = modules_dir / module
        if not (module_dir / "include").is_dir():
            print(f"FAIL: reusable module include directory not found: {module_dir / 'include'}", file=sys.stderr)
            return 1
        for source_dir in (module_dir / "include", module_dir / "src"):
            if not source_dir.is_dir():
                continue
            for source in sorted(source_dir.glob("**/*")):
                if source.suffix not in (".h", ".hpp", ".cpp"):
                    continue
                rel = source.relative_to(root)
                for line_no, line in enumerate(source.read_text(encoding="utf-8").splitlines(), 1):
                    if is_comment_only(line):
                        continue
                    code = strip_line_comment(line)
                    if PATTERN.search(code):
                        findings.append(f"{rel}:{line_no}: {line.strip()}")

    if findings:
        print("FAIL: JUCE reference found in reusable core headers", file=sys.stderr)
        for finding in findings:
            print(finding, file=sys.stderr)
        return 1

    print(f"PASS: core framework boundary holds ({len(REUSABLE_MODULES)} modules checked).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
