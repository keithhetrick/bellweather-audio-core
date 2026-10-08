# Copyright (c) 2026 Bellweather Studios.
# SPDX-License-Identifier: Apache-2.0

"""G384-1/4 integration: closed-form oracle and native mutation detection.

Risk: wrong envelope dynamics or binary transport can yield plausible metrics.
The oracle is closed-form, not a second recurrence. Actual native mutation,
malformed inputs, channel twins and restored source exercise detection power.
"""

import shutil
import sys

import numpy as np
from reproduce import build, digest, invoke, render, write


def qualify(source, out, binary):
    params = [44100, -24, 4, 0.01, 0.3, 0]

    def run(name, samples, executable=binary, configuration=params):
        inp = out / (name + ".input.f64")
        samples.astype("<f8").tofile(inp)
        dest = out / (name + ".gain.f64")
        return render(
            [
                str(executable),
                str(inp),
                str(dest),
                str(1 if samples.ndim == 1 else 2),
                *[str(v) for v in configuration],
            ],
            dest,
            len(samples),
            out / (name + ".log"),
        )

    x = np.concatenate([np.ones(4410), np.zeros(4410)])
    first = -18 * (1 - np.exp(-np.arange(1, 4411) / 441.0))
    oracle = np.concatenate([first, first[-1] * np.exp(-np.arange(1, 4411) / 13230.0)])
    actual = run("step", x)
    error = float(np.max(np.abs(actual - oracle)))
    if error >= 1e-9:
        raise AssertionError("G384-4: analytic native error")
    for name, stereo in [
        ("left", np.column_stack([x, x * 0])),
        ("right", np.column_stack([x * 0, x])),
    ]:
        if not np.array_equal(run(name, stereo), actual):
            raise AssertionError("G384-4: stereo link")
    if not np.array_equal(run("reset", x), actual):
        raise AssertionError("G384-4: invocation reset")
    if (
        np.max(
            np.abs(
                run(
                    "silence",
                    np.zeros(4410),
                    configuration=[44100, -24, 4, 0.01, 0.3, 0.25],
                )
                - 0.25
            )
        )
        >= 1e-12
    ):
        raise AssertionError("G384-4: silence makeup")
    for name, data in [
        ("truncated", bytes(1)),
        ("nan", np.array([np.nan], dtype="<f8").tobytes()),
    ]:
        inp = out / (name + ".input.f64")
        inp.write_bytes(data)
        command = [
            str(binary),
            str(inp),
            str(out / (name + ".gain.f64")),
            "1",
            *[str(v) for v in params],
        ]
        if invoke(command, out, out / (name + ".log"), allow_failure=True) == 0:
            raise AssertionError("G384-1: invalid input accepted")
    # G384-1: each rejected boundary has an unaffected valid step control above.
    for name, index, invalid in [
        ("channels", 0, "3"),
        ("sample-rate", 1, "0"),
        ("ratio", 3, ".5"),
        ("attack", 4, "0"),
        ("release", 5, "-1"),
        ("finite", 2, "nan"),
        ("syntax", 2, "-24junk"),
    ]:
        values = ["1", *[str(v) for v in params]]
        values[index] = invalid
        command = [
            str(binary),
            str(out / "step.input.f64"),
            str(out / (name + ".rejected.f64")),
            *values,
        ]
        if invoke(command, out, out / (name + ".rejected.log"), allow_failure=True) == 0:
            raise AssertionError("G384-1: invalid configuration accepted: " + name)
    existing = out / "step.gain.f64"
    before = digest(existing)
    try:
        render(
            [
                str(binary),
                str(out / "step.input.f64"),
                str(existing),
                "1",
                *[str(v) for v in params],
            ],
            existing,
            len(x),
            out / "overwrite.log",
        )
    except ValueError:
        if digest(existing) != before:
            raise AssertionError("G384-4: existing output modified") from None
    else:
        raise AssertionError("G384-4: existing output accepted")
    mutant = out / "mutant-source"
    mutant.mkdir()
    for name in ["CMakeLists.txt", "Baseline.h", "render.cpp"]:
        shutil.copyfile(source / name, mutant / name)
    header = (mutant / "Baseline.h").read_text()
    if header.count("rate * attack") != 1 or header.count("rate * release") != 1:
        raise AssertionError("G384-4: mutation not activated")
    (mutant / "Baseline.h").write_text(
        header.replace("rate * attack", "rate * attack * 2.0").replace(
            "rate * release", "rate * release * 2.0"
        )
    )
    mutated = build(mutant, out, "mutant")
    mutant_error = float(np.max(np.abs(run("mutant", x, mutated) - oracle)))
    if mutant_error < 1e-9:
        raise AssertionError("G384-4: mutant escaped")
    if np.max(np.abs(run("restored", x) - oracle)) >= 1e-9:
        raise AssertionError("G384-4: restoration failed")
    for name, code in [
        (
            "failed",
            "import pathlib,sys; pathlib.Path(sys.argv[1]).write_bytes(bytes(8)); sys.exit(7)",
        ),
        ("missing", "pass"),
    ]:
        dest = out / (name + ".f64")
        try:
            render([sys.executable, "-c", code, str(dest)], dest, 1, out / (name + ".log"))
        except (RuntimeError, ValueError):
            pass
        else:
            raise AssertionError("G384-4: failed/missing output accepted")
    write(
        out / "controls.json",
        {
            "analytic_max_error_dB": error,
            "mutant_max_error_dB": mutant_error,
            "restored": True,
            "transport": "passed",
        },
    )
