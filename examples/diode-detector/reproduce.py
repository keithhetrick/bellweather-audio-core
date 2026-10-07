#!/usr/bin/env python3
# Copyright (c) 2026 Bellweather Studios.
# SPDX-License-Identifier: Apache-2.0

"""Build and reproduce the diode circuit experiment in a new external directory."""

import argparse
import hashlib
import json
import os
import platform
import sys
import time
from pathlib import Path

sys.dont_write_bytecode = True


HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def source_identity():
    closure = json.loads((HERE / "circuit_files.json").read_text())
    files = {name: digest(HERE / name) for name in closure}
    for relative in ("cmake/BwsCatch2.cmake", "docs/testing/contracts/diode-detector-modeling.md"):
        files[relative] = digest(ROOT / relative)
    return files


def completed_outcome(code: int, reported: str) -> str:
    """DDM-7: non-pass execution cannot certify a passing numerical report."""
    if code == 0 and reported == "passed":
        return "passed"
    if code == 1 and reported == "inconclusive":
        return "inconclusive"
    return "failed"


def main():
    from circuit_process import install_handlers, invoke

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    out = args.output.resolve()
    if out == ROOT or ROOT in out.parents:
        parser.error("output must be outside the source tree")
    try:
        out.mkdir(parents=True, exist_ok=False)
    except FileExistsError:
        parser.error("output already exists; choose a new directory")
    started = time.monotonic()
    result = {
        "outcome": "running",
        "stage": "dependencies",
        "command": sys.argv,
        "python": sys.version,
        "platform": platform.platform(),
    }
    install_handlers()
    os.environ["DISABLE_EMAIL_SENDING_FOR_TESTS"] = "1"
    os.environ["PYTHONDONTWRITEBYTECODE"] = "1"
    os.environ["BWS_CIRCUIT_CONTROLS"] = str(out / "simulator-controls")
    for name, folder in (
        ("MPLCONFIGDIR", "matplotlib"),
        ("XDG_CACHE_HOME", "cache"),
        ("TMPDIR", "tmp"),
    ):
        target = out / folder
        target.mkdir()
        os.environ[name] = str(target)

    def run(stage, command, timeout=180, allow_failure=False):
        result["stage"] = stage
        return invoke(
            command, out, out / f"{stage}.log", timeout=timeout, allow_failure=allow_failure
        )

    try:
        result["source_sha256"] = source_identity()
        run(
            "dependencies",
            [
                sys.executable,
                "-c",
                "import numpy,scipy,matplotlib; print(numpy.__version__,scipy.__version__,matplotlib.__version__)",
            ],
        )
        run("simulator-version", ["ngspice", "--version"])
        run(
            "configure",
            [
                "cmake",
                "-S",
                str(HERE),
                "-B",
                str(out / "build"),
                "-G",
                "Ninja",
                "-DCMAKE_BUILD_TYPE=Release",
                "-DBUILD_TESTING=ON",
            ],
            900,
        )
        run("build", ["cmake", "--build", str(out / "build"), "--parallel", "4"], 900)
        run("interface", ["ctest", "--test-dir", str(out / "build"), "--output-on-failure"])
        for name in (
            "checks_test.py",
            "circuit_process_test.py",
            "simulator_test.py",
            "reproduce_test.py",
        ):
            run(name.removesuffix(".py"), [sys.executable, str(HERE / name)])
        run("reference", [sys.executable, str(HERE / "reference.py")])
        code = run(
            "experiment",
            [
                sys.executable,
                str(HERE / "run_experiment.py"),
                "--binary",
                str(out / "build" / "bws-diode-render"),
                "--out",
                str(out / "experiment"),
            ],
            3600,
            allow_failure=True,
        )
        report = json.loads((out / "experiment" / "results.json").read_text())
        result["experiment_outcome"] = report["outcome"]
        result["outcome"] = completed_outcome(code, report["outcome"])
        if result["outcome"] != "passed":
            result["recovery"] = (
                "Inspect experiment/results.json and raw diagnostics; preserve this run before retrying."
            )

    except BaseException as exc:
        result.update(
            outcome="failed",
            error=f"{type(exc).__name__}: {exc}",
            recovery="Inspect the failed-stage log; correct the cause and use a new output directory.",
        )
        report_file = out / "experiment" / "results.json"
        if report_file.exists():
            result["experiment_outcome"] = json.loads(report_file.read_text())["outcome"]
    finally:
        try:
            result["source_after_sha256"] = source_identity()
            if result.get("source_sha256") != result["source_after_sha256"]:
                result.update(outcome="failed", error="Source identity changed during execution")
        except Exception as exc:
            result.update(outcome="failed", error=f"Source identity unavailable: {exc}")
        result["elapsed_seconds"] = time.monotonic() - started
        (out / "results.json").write_text(json.dumps(result, indent=2) + "\n")
        files = {
            str(p.relative_to(out)): digest(p)
            for p in out.rglob("*")
            if p.is_file()
            and "build" not in p.relative_to(out).parts
            and "cache" not in p.relative_to(out).parts
            and p.name != "manifest.json"
        }
        (out / "manifest.json").write_text(json.dumps(files, indent=2) + "\n")
    print(f"{result['outcome']}: {out / 'results.json'}")
    return 0 if result["outcome"] == "passed" else 1


if __name__ == "__main__":
    sys.exit(main())
