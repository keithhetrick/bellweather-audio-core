#!/usr/bin/env python3
# Copyright (c) 2026 Bellweather Studios.
# SPDX-License-Identifier: Apache-2.0

"""DDM-9: reproduce a correct, source-mutated and restored public circuit."""

import argparse
import json
import shutil
import sys
from pathlib import Path

sys.dont_write_bytecode = True


def main():
    from circuit_process import install_handlers, invoke
    from reproduce import HERE, ROOT, digest, source_identity

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    out = args.output.resolve()
    if out == ROOT or ROOT in out.parents:
        parser.error("output must be outside the source tree")
    out.mkdir(parents=True, exist_ok=False)
    install_handlers()
    result = {"outcome": "running", "runs": {}}
    try:
        before = source_identity()
        for case in ("correct", "mutated", "restored"):
            if case == "correct":
                entry = HERE / "reproduce.py"
            else:
                source = out / f"{case} source"
                example = source / "examples/diode-detector"
                for name in json.loads((HERE / "circuit_files.json").read_text()):
                    target = example / name
                    target.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copyfile(HERE / name, target)
                for name in (
                    "cmake/BwsCatch2.cmake",
                    "docs/testing/contracts/diode-detector-modeling.md",
                ):
                    target = source / name
                    target.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copyfile(ROOT / name, target)
                if case == "mutated":
                    model = example / "DiodeDetector.h"
                    original = model.read_text()
                    needle = "charge_ = capacitance * rate;"
                    if original.count(needle) != 1:
                        raise RuntimeError("DDM-9 mutation site changed; review the named mutation")
                    model.write_text(
                        original.replace(needle, "charge_ = 2.0 * capacitance * rate;")
                    )
                entry = example / "reproduce.py"
            work = out / f"{case} run"
            code = invoke(
                [sys.executable, "-B", str(entry), str(work)],
                out,
                out / f"{case}.log",
                timeout=7200,
                allow_failure=True,
            )
            report = json.loads((work / "results.json").read_text())
            result["runs"][case] = {
                "exit": code,
                "outcome": report["outcome"],
                "report_sha256": digest(work / "results.json"),
            }
            if case == "mutated":
                numerical = json.loads((work / "experiment/results.json").read_text())
                if (
                    code == 0
                    or numerical["outcome"] != "failed"
                    or not any(
                        "native/Python disagreement" in failure for failure in numerical["failures"]
                    )
                ):
                    raise RuntimeError("DDM-9: mutation did not reach its numerical oracle")
            elif code != 0 or report["outcome"] != "passed":
                raise RuntimeError(f"DDM-9: {case} control did not pass")
        if source_identity() != before:
            raise RuntimeError("DDM-9: original source changed")
        result["source_sha256"] = before
        result["outcome"] = "passed"
    except BaseException as exc:
        result.update(outcome="failed", error=f"{type(exc).__name__}: {exc}")
    (out / "qualification.json").write_text(json.dumps(result, indent=2) + "\n")
    print(f"{result['outcome']}: {out / 'qualification.json'}")
    return 0 if result["outcome"] == "passed" else 1


if __name__ == "__main__":
    sys.exit(main())
