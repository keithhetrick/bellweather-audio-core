#!/usr/bin/env python3
# Copyright (c) 2026 Bellweather Studios.
# SPDX-License-Identifier: Apache-2.0

"""G384-1..7: reproduce the fixed hardware gain comparison in a new directory."""

import argparse
import hashlib
import importlib
import json
import os
import platform
import shutil
import signal
import sys
import time
import urllib.request
import warnings
from pathlib import Path

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
# Canonical shared helper lives in the parent; export copies that same helper.
SUPPORT = HERE if (HERE / "circuit_process.py").exists() else HERE.parent
sys.path.append(str(SUPPORT))
_process = importlib.import_module("circuit_process")
invoke = _process.invoke
install_handlers = _process.install_handlers

IDS = ["clip_00131", "clip_00143", "clip_00209"]


def digest(path):
    h = hashlib.sha256()
    with Path(path).open("rb") as f:
        for block in iter(lambda: f.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def write(path, value):
    path.write_text(json.dumps(value, indent=2, allow_nan=False) + "\n")


def identities():
    paths = [HERE / name for name in json.loads((HERE / "files.json").read_text())]
    paths += [SUPPORT / "circuit_process.py", SUPPORT / "requirements.lock"]
    return {
        str(p.relative_to(HERE)) if p.is_relative_to(HERE) else "../" + p.name: digest(p)
        for p in paths
    }


def metrics(a, b):
    import numpy as np

    if (
        a.ndim != 1
        or a.shape != b.shape
        or not a.size
        or not np.isfinite(a).all()
        or not np.isfinite(b).all()
    ):
        raise ValueError("G384-3: invalid metric inputs")
    d = a - b
    return dict(
        mae_dB=float(np.mean(np.abs(d))),
        rms_dB=float(np.sqrt(np.mean(d * d))),
        peak_dB=float(np.max(np.abs(d))),
        bias_dB=float(np.mean(d)),
    )


def reconstruct(x, gain, cv):
    """G384-7: apply native and recorded gains, never play CV as audio."""
    import numpy as np

    if (
        x.ndim != 2
        or x.shape[1] != 2
        or len(x) < 3
        or gain.shape != (len(x),)
        or cv.shape != gain.shape
        or not all(np.isfinite(v).all() for v in (x, gain, cv))
    ):
        raise ValueError("G384-7: invalid audio/gain framing or values")
    return (
        x[:-2] * np.power(10.0, gain[:-2, None] / 20.0),
        x[:-2] * np.power(10.0, cv[2:, None] / 20.0),
    )


def listening_gains(tracks):
    """G384-7: constant stereo RMS matching with shared peak attenuation."""
    import numpy as np

    if not tracks or any(
        v.ndim != 2
        or v.shape[1] != 2
        or not v.size
        or v.shape != tracks[0].shape
        or not np.isfinite(v).all()
        for v in tracks
    ):
        raise ValueError("G384-7: invalid listening tracks")
    rms = np.array([np.sqrt(np.mean(v * v)) for v in tracks])
    if not np.isfinite(rms).all() or np.any(rms <= 0):
        raise ValueError("G384-7: silent or invalid listening track")
    gains = np.min(rms) / rms
    peak = max(np.max(np.abs(v)) * g for v, g in zip(tracks, gains, strict=True))
    return gains * min(1.0, 10 ** (-3 / 20) / peak)


def audition(out, clip, x, gain, cv, hardware):
    import numpy as np
    from scipy.io import wavfile

    model, recorded_gain = reconstruct(x, gain, cv)
    tracks = [x[:-2], model, hardware[:-2], recorded_gain]
    gains = listening_gains(tracks)
    names = ["01-input", "02-model-gain-only", "03-recorded-hardware", "04-hardware-gain-only"]
    receipt = {"frames": len(x) - 2, "rate": 44100, "tracks": {}}
    for name, samples, scale in zip(names, tracks, gains, strict=True):
        item = {"listening_gain": float(scale), "sha256": {}}
        for folder, factor in [("measurement", 1.0), ("listen", scale)]:
            path = out / clip / folder / (name + ".wav")
            path.parent.mkdir(parents=True, exist_ok=True)
            expected = (samples * factor).astype(np.float32)
            if not np.isfinite(expected).all():
                raise ValueError("G384-7: nonfinite rendered audio")
            wavfile.write(path, 44100, expected)
            rate, actual = wavfile.read(path)
            if rate != 44100 or not np.array_equal(actual, expected):
                raise ValueError("G384-7: audio write/readback mismatch")
            item["sha256"][folder] = digest(path)
        receipt["tracks"][name] = item
    return receipt


def compare(gain, cv):
    """G384-3: one fixed alignment and one partition for every comparator."""
    import numpy as np

    if (
        gain.ndim != 1
        or gain.shape != cv.shape
        or gain.size < 3
        or not np.isfinite(gain).all()
        or not np.isfinite(cv).all()
    ):
        raise ValueError("G384-3: invalid complete comparison records")
    g, target = gain[:-2], cv[2:]
    windows = [
        {
            "start_frame": n,
            "frames": len(target[n : n + 44100]),
            **metrics(g[n : n + 44100], target[n : n + 44100]),
        }
        for n in range(0, len(target), 44100)
    ]
    return metrics(g, target), windows


def verdict(rows, integrity=True):
    import math

    try:
        if not integrity or sorted(r["id"] for r in rows) != IDS:
            return "evidence_error"
        if not all(
            math.isfinite(r[k][m]) and r[k][m] >= 0
            for r in rows
            for k in ["nominal", "fitted"]
            for m in ["mae_dB", "rms_dB"]
        ):
            return "evidence_error"
        return (
            "improved_on_all_records"
            if all(r["fitted"][m] < r["nominal"][m] for r in rows for m in ["mae_dB", "rms_dB"])
            else "mixed_or_not_improved"
        )
    except (KeyError, TypeError, ValueError):
        return "evidence_error"


def build(source, out, name):
    directory = out / (name + "-build")
    for stage, cmd in [
        (
            "configure",
            [
                "cmake",
                "-S",
                str(source),
                "-B",
                str(directory),
                "-DCMAKE_BUILD_TYPE=Release",
            ],
        ),
        ("build", ["cmake", "--build", str(directory), "--parallel", "2"]),
    ]:
        write(out / (name + "-" + stage + ".command.json"), cmd)
        invoke(cmd, out, out / (name + "-" + stage + ".log"), timeout=900)
    return directory / "g384-baseline"


def render(command, output, count, log):
    import numpy as np

    if output.exists():
        raise ValueError("G384-4: existing renderer output; choose a fresh directory")
    write(log.with_suffix(".command.json"), command)
    invoke(command, log.parent, log, timeout=180)
    if not output.is_file() or output.stat().st_size != count * 8:
        raise ValueError("G384-4: missing/malformed renderer output; inspect child log")
    g = np.fromfile(output, dtype="<f8")
    if not np.isfinite(g).all():
        raise ValueError("G384-4: nonfinite renderer output")
    return g


def acquire(out, reuse):
    manifest = json.loads((HERE / "dataset.json").read_text())
    data = out / "data"
    data.mkdir()
    acquired = []
    for item in manifest["files"]:
        dest = data / item["path"]
        dest.parent.mkdir(parents=True, exist_ok=True)
        if reuse is not None:
            source = reuse / item["path"]
            if source.stat().st_size != item["bytes"] or digest(source) != item["sha256"]:
                raise ValueError("G384-2: invalid cached file " + str(source))
            shutil.copyfile(source, dest)
        else:
            with urllib.request.urlopen(item["url"], timeout=60) as response, dest.open("xb") as f:
                remaining = item["bytes"] + 1
                while remaining:
                    chunk = response.read(min(1024 * 1024, remaining))
                    if not chunk:
                        break
                    f.write(chunk)
                    remaining -= len(chunk)
        if dest.stat().st_size != item["bytes"] or digest(dest) != item["sha256"]:
            raise ValueError("G384-2: dataset identity failure " + item["path"])
        acquired.append(item)
        write(
            out / "acquisition.json",
            {
                "revision": manifest["revision"],
                "files": acquired,
                "requests_max": len(manifest["files"]),
                "expected_bytes": manifest["total_bytes"],
            },
        )
    return data, manifest


def decode(path, shape, pcm):
    import numpy as np
    from scipy.io import wavfile

    with warnings.catch_warnings(record=True) as diagnostics:
        warnings.simplefilter("always")
        rate, x = wavfile.read(path)
    messages = [str(d.message) for d in diagnostics]
    if any(
        pcm or message != "Chunk (non-data) not understood, skipping it." for message in messages
    ):
        raise ValueError("G384-2: unrecognized WAV diagnostic: " + repr(messages))
    if messages:
        write(
            path.with_suffix(".decoding.json"),
            {"admitted_metadata": "PEAK in pinned CV", "warnings": messages},
        )
    if pcm:
        if x.dtype != np.int32:
            raise ValueError("G384-2: expected left-justified 24-bit PCM")
        x = x.astype(np.float64) / 2147483648.0
    else:
        if x.dtype.kind != "f":
            raise ValueError("G384-2: expected float CV")
        x = x.astype(np.float64)
    if rate != 44100 or x.shape != shape or not np.isfinite(x).all():
        raise ValueError("G384-2: invalid rate/framing/values")
    return x


def run(out, reuse=None):
    if out.exists() or out.is_relative_to(HERE):
        raise ValueError("G384-5: choose an absent output directory outside source")
    out.mkdir(parents=True)
    started = time.monotonic()
    result = {
        "verdict": "evidence_error",
        "stage": "dependencies",
        "rows": [],
        "environment": {"python": sys.version, "platform": platform.platform()},
        "claim": "fixed three-record gain prediction only; not full audio emulation",
    }
    install_handlers()

    def expired(*_):
        raise TimeoutError("G384-5: 3600-second run limit exceeded")

    signal.signal(signal.SIGALRM, expired)
    signal.alarm(3600)
    os.environ["PYTHONDONTWRITEBYTECODE"] = "1"
    try:
        import numpy as np
        import scipy

        result["environment"].update(numpy=np.__version__, scipy=scipy.__version__)
        before = identities()
        write(out / "source-identities.json", before)
        candidate = json.loads((HERE / "candidate.json").read_text())
        write(out / "candidate.json", candidate)
        result["stage"] = "instrument qualification"
        for script in ["evaluation_test.py", "lifecycle_test.py"]:
            invoke([sys.executable, str(HERE / script)], out, out / (script + ".log"))
        binary = build(HERE, out, "native")
        from native_test import qualify

        controls = out / "controls"
        controls.mkdir()
        qualify(HERE, controls, binary)
        result["stage"] = "reference acquisition"
        data, manifest = acquire(out, reuse)
        result["stage"] = "hardware evaluation"
        params = candidate["parameters"]
        names = ["threshold_dB", "ratio", "attack_s", "release_s"]
        configurations = {
            "nominal": [candidate["nominal"][k] for k in names],
            "fitted": [params[k] for k in names],
        }
        for clip in IDS:
            x = decode(data / f"audio_in/{clip}.wav", (1323000, 2), True)
            target = decode(data / f"cv/p063/{clip}_p063.wav", (1323000,), False)
            inp = out / (clip + ".input.f64")
            x.astype("<f8").tofile(inp)
            row = {"id": clip, "windows": {}}
            for name, p in configurations.items():
                dest = out / (clip + "." + name + ".f64")
                command = [
                    str(binary),
                    str(inp),
                    str(dest),
                    "2",
                    "44100",
                    *[str(v) for v in p],
                    str(candidate["makeup_dB"]),
                ]
                g = render(command, dest, len(x), out / (clip + "." + name + ".log"))
                row[name], row["windows"][name] = compare(g, target)
                if name == "fitted":
                    hardware = decode(data / f"audio_out/p063/{clip}_p063.wav", x.shape, True)
                    row["audition"] = audition(out, clip, x, g, target, hardware)
            g = np.full(target.shape, candidate["makeup_dB"])
            row["constant_makeup"], row["windows"]["constant_makeup"] = compare(g, target)
            result["rows"].append(row)
            write(out / "results.json", result)
        (out / "LISTEN.md").write_text(
            "# G384 listening comparison\n\n"
            "Open each clip's listen/ WAV files in a 44.1 kHz stereo session. "
            "Align their starts, disable processing/normalization/time stretching, "
            "and solo one track at a time at a comfortable volume.\n\n"
            "01 is input; 02 is input with predicted model gain; 03 is recorded "
            "hardware output; 04 is input with recorded hardware gain. "
            "02 and 04 omit analog coloration. The recorded hardware track has "
            "no fitted delay alignment; do not interpret a null test as fidelity.\n\n"
            "listen/ uses whole-record stereo RMS matching and constant attenuation. "
            "measurement/ preserves unscaled samples. Neither is a perceptual test. "
            "See results.json for numerical verdict, track hashes and gains; "
            "listening never changes that verdict.\n"
        )
        if identities() != before:
            raise ValueError("G384-5: source changed during run")
        for item in manifest["files"]:
            if digest(data / item["path"]) != item["sha256"]:
                raise ValueError("G384-5: data changed during run")
        result.update(stage="complete", verdict=verdict(result["rows"]), identities_unchanged=True)
    except BaseException as exc:
        result.update(
            verdict="evidence_error",
            error=f"{type(exc).__name__}: {exc}",
            recovery="Inspect this stage log; correct the prerequisite and rerun in a fresh output directory.",
        )
    finally:
        signal.alarm(0)
        result["elapsed_seconds"] = time.monotonic() - started
        result["storage_bytes_before_final_receipt"] = sum(
            p.stat().st_size for p in out.rglob("*") if p.is_file()
        )
        write(out / "results.json", result)
    print(json.dumps({"verdict": result["verdict"], "stage": result["stage"], "output": str(out)}))
    return {
        "improved_on_all_records": 0,
        "mixed_or_not_improved": 2,
        "evidence_error": 1,
    }[result["verdict"]]


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument(
        "--data",
        type=Path,
        help="Verified offline dataset directory (audio_in/ and cv/).",
    )
    args = parser.parse_args()
    sys.exit(run(args.output.resolve(), args.data.resolve() if args.data else None))
