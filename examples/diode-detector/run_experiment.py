#!/usr/bin/env python3
# Copyright (c) 2026 Bellweather Studios.
# SPDX-License-Identifier: Apache-2.0

"""DDM-1..5: retain a reproducible nonlinear circuit experiment in an absent directory."""

import argparse
import hashlib
import json
import platform
import sys
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import scipy
from checks import (
    check_convergence,
    check_dc,
    check_negative,
    check_port,
    check_source,
    errors,
    require,
)
from circuit_process import install_handlers, invoke
from reference import dc_voltage
from reference import render as python_render
from reproduce import source_identity
from simulator import check_diagnostics

ROOT = Path(__file__).resolve().parent
REPO = ROOT.parent.parent


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def stimulus(kind, duration, frequency):
    stop = 0.01 + duration + 0.2
    if kind == "zero":
        return np.array([0.0, stop]), np.zeros(2), stop
    if kind == "dc":
        # Explicit finite source edge, identical in SPICE and sampled input.
        stop = 0.51
        return np.array([0, 0.01, 0.01 + 1 / 768000, stop]), np.array([0, 0, 2, 2]), stop
    t = np.arange(round(duration * 768000) + 1) / 768000
    x = 2 * np.sin(2 * np.pi * frequency * t)
    x[0] = x[-1] = 0
    return np.concatenate(([0], t + 0.01, [stop])), np.concatenate(([0], x, [0])), stop


def spice(case, times, values, stop, method, maxstep):
    stem = f"{method}-{maxstep:g}"
    net = case / f"{stem}.cir"
    net.write_text(f"""Diode detector DDM-1
.include source.inc
Rs in anode 1000
D1 anode out DETECTOR
RL out 0 100000
C1 out 0 1u IC=0
.model DETECTOR D(Is=1e-12 N=1 Rs=0 Cjo=0 Tt=0 Tnom=26.85)
.temp 26.85
.options method={method} reltol=1e-9 abstol=1e-14 vntol=1e-11 gmin=1e-15
.control
set noaskquit
set wr_singlescale
set wr_vecnames
set numdgt=15
save v(out) v(in)
tran {1 / 96000:.17g} {stop:.17g} 0 {maxstep:.17g}
wrdata {stem}-source.txt v(in)
linearize v(out)
wrdata {stem}.txt v(out)
quit
.endc
.end
""")
    invoke(["ngspice", "-n", "-b", net.name], case, case / f"{stem}.log")
    check_diagnostics((case / f"{stem}.log").read_text())
    data = np.loadtxt(case / f"{stem}.txt", skiprows=1)
    require(data.ndim == 2 and data.shape[1] == 2 and len(data) > 1, "Missing simulator samples")
    require(
        np.isfinite(data).all() and np.all(np.diff(data[:, 0]) > 0), "Invalid simulator time series"
    )
    require(
        data[0, 0] <= 1e-10 and data[-1, 0] >= stop - 2 / 96000, "Incomplete simulator duration"
    )
    source = np.loadtxt(case / f"{stem}-source.txt", skiprows=1)
    measured = check_source(source[:, 1], np.interp(source[:, 0], times, values))
    (case / f"{stem}-source-check.json").write_text(json.dumps(measured, indent=2))
    return data


def native(binary, case, rate, capacitance, times, values, stop):
    grid = np.arange(round(stop * rate) + 1) / rate
    samples = np.interp(grid[1:], times, values)
    stem = f"native-{rate}-{capacitance:g}"
    np.savetxt(case / f"{stem}-input.txt", samples, fmt="%.17g")
    with (case / f"{stem}-input.txt").open() as inp, (case / f"{stem}.csv").open("w") as out:
        invoke([str(binary), str(rate), str(capacitance)], case, case / f"{stem}.log", inp, out)
    data = np.loadtxt(case / f"{stem}.csv", delimiter=",", skiprows=1, ndmin=2)
    require(data.shape == (len(samples), 3), "Native output shape mismatch")
    require(
        np.isfinite(data).all() and np.all(data[:, 2] == 1) and np.max(data[:, 1]) <= 48,
        "DDM-2: rejected/nonfinite/unbounded native render",
    )
    return grid, np.concatenate(([0.0], data[:, 0])), samples, int(np.max(data[:, 1]))


def experiment(binary, out, result):
    failures, inconclusive = [], []

    def check(label, function, *args):
        try:
            return function(*args)
        except RuntimeError as exc:
            failures.append(f"{label}: {exc}")
            return None

    configs = [
        ("zero", 0.01, 0),
        ("dc", 0.3, 0),
        ("burst10", 0.01, 1000),
        ("burst100", 0.1, 1000),
        ("burst1000", 1.0, 1000),
        ("hf18k", 0.1, 18000),
    ]
    fig, axes = plt.subplots(len(configs), 2, figsize=(13, 16))
    for row, (name, duration, frequency) in enumerate(configs):
        print(f"Running {name}", flush=True)
        case = out / name
        case.mkdir()
        times, values, stop = stimulus(name, duration, frequency)
        np.savetxt(
            case / "source.csv",
            np.column_stack((times, values)),
            delimiter=",",
            header="time_s,voltage_V",
        )
        if frequency:
            # Algebraically evaluate the same sampled-sine PWL. A million-pair
            # voltage-source table makes ngspice's lookup path prohibitively slow.
            omega = 2 * np.pi * frequency / 768000
            (case / "source.inc").write_text(f""".func pos(t) {{768000*(t-0.01)}}
.func fraction(t) {{pos(t)-floor(pos(t))}}
.func sampled(t) {{2*((1-fraction(t))*sin({omega:.17g}*floor(pos(t)))+fraction(t)*sin({omega:.17g}*(floor(pos(t))+1)))}}
B1 in 0 V=(time >= 0.01 && time < {0.01 + duration:.17g}) ? sampled(time) : 0
""")
        else:
            points = " ".join(f"{t:.17g} {v:.17g}" for t, v in zip(times, values, strict=True))
            (case / "source.inc").write_text(f"V1 in 0 PWL({points})\n")
        coarse = spice(case, times, values, stop, "trap", 2e-6)
        fine = spice(case, times, values, stop, "trap", 0.5e-6)
        gear = spice(case, times, values, stop, "gear", 0.5e-6)
        grid = fine[:, 0]
        comparisons = [
            errors(np.interp(grid, data[:, 0], data[:, 1]), fine[:, 1]) for data in [coarse, gear]
        ]
        reference_error = max(x["max_V"] for x in comparisons)
        if reference_error > 1e-4:
            inconclusive.append(f"{name}: SPICE uncertainty exceeds DDM-4 budget")
        entry = {"reference_comparisons": comparisons, "digital": []}
        result["cases"][name] = entry
        by_rate = {}
        for rate in sorted(
            {base * factor for base in [44100, 48000, 96000] for factor in [1, 2, 4]}
        ):
            t, y, inputs, iterations = native(binary, case, rate, 1e-6, times, values, stop)
            # Compare on the shared observation grid, excluding no transient.
            measured = errors(np.interp(grid, t, y), fine[:, 1])
            by_rate[rate] = measured["rms_V"]
            entry["digital"].append(
                {"rate_Hz": rate, "error": measured, "max_iterations": iterations}
            )
            if rate == 48000:
                if name in ("zero", "dc", "burst100"):
                    reference = python_render(inputs, rate)
                    np.savetxt(case / "python-48000.csv", reference, header="voltage_V")
                    entry["port_parity"] = check(name, check_port, y[1:], reference)
                if name == "zero":
                    check(name, require, np.count_nonzero(y) == 0, "DDM-2: zero native output")
                if name == "dc":
                    entry["DC_expected_V"] = dc_voltage()
                    entry["DC_observed_V"] = float(y[-1])
                    check(name, check_dc, y[-1], dc_voltage())
                if name == "burst100":
                    wt, wy, _, _ = native(binary, case, rate, 2e-6, times, values, stop)
                    wrong = errors(np.interp(grid, wt, wy), fine[:, 1])
                    entry["wrong_capacitance_error"] = wrong
                    check(name, check_negative, measured["rms_V"], wrong["rms_V"])
                    axes[row, 0].plot(wt, wy, ":", label="Wrong C (2x)")
                axes[row, 0].plot(t, y, label="Native 48 kHz")
        if frequency == 1000:
            for base in [44100, 48000, 96000]:
                if reference_error > 0.1 * by_rate[base]:
                    inconclusive.append(f"{name}/{base}: reference floor too large")
                resolved = check(
                    f"{name}/{base}",
                    check_convergence,
                    by_rate[base],
                    by_rate[base * 4],
                    reference_error,
                )
                if resolved is False:
                    inconclusive.append(
                        f"{name}/{base}: improvement unresolved against reference disagreement"
                    )
        axes[row, 0].plot(grid, fine[:, 1], "--", label="SPICE")
        axes[row, 0].set(title=name, xlabel="Time (s)", ylabel="Capacitor voltage (V)")
        axes[row, 0].legend()
        axes[row, 1].loglog(list(by_rate), [max(v, 1e-16) for v in by_rate.values()], "o-")
        axes[row, 1].set(xlabel="Integration rate (Hz)", ylabel="RMS error (V, floor 1e-16)")
        (out / "progress.json").write_text(json.dumps(result, indent=2))
    fig.tight_layout()
    fig.savefig(out / "comparison.png", dpi=120)
    result.update(
        failures=failures,
        inconclusive=inconclusive,
        outcome="failed" if failures else "inconclusive" if inconclusive else "passed",
    )


def main():
    install_handlers()
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    binary = args.binary.resolve()
    out = args.out.absolute()
    out.mkdir(parents=True, exist_ok=False)
    result = {
        "contract": "docs/testing/contracts/diode-detector-modeling.md",
        "cases": {},
        "command": sys.argv,
        "python": sys.version,
        "numpy": np.__version__,
        "scipy": scipy.__version__,
        "platform": platform.platform(),
        "binary": str(binary),
        "outcome": "running",
    }
    try:
        result["binary_sha256"] = digest(binary)
        result["source_sha256"] = source_identity()
        result["contract_sha256"] = digest(REPO / result["contract"])
        invoke(["ngspice", "--version"], out, out / "ngspice-version.log")
        result["ngspice"] = (out / "ngspice-version.log").read_text()
        experiment(binary, out, result)
    except BaseException as exc:
        result.update(outcome="failed", error=f"{type(exc).__name__}: {exc}")
    finally:
        (out / "results.json").write_text(json.dumps(result, indent=2, allow_nan=False) + "\n")
        files = {str(p.relative_to(out)): digest(p) for p in out.rglob("*") if p.is_file()}
        (out / "manifest.json").write_text(json.dumps(files, indent=2) + "\n")
    print(
        json.dumps(
            {k: result[k] for k in ["outcome", "failures", "inconclusive", "error"] if k in result},
            indent=2,
        )
    )
    return 0 if result["outcome"] == "passed" else 1


if __name__ == "__main__":
    sys.exit(main())
