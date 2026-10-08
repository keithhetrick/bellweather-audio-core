#!/usr/bin/env python3
# Copyright (c) 2026 Bellweather Studios.
# SPDX-License-Identifier: Apache-2.0

"""G384-6: full reproduction, replay, data rejection and fresh-directory recovery.

Integration is the smallest level proving real dataset/renderer transport.
Whole workflows additionally prove command entry, preserved evidence and replay.
Invalid data is a corrupt-copy witness; the valid cache remains unaffected.
"""

import argparse
import importlib
import json
import os
import shlex
import shutil
import signal
import subprocess
import sys
import time
from pathlib import Path

sys.dont_write_bytecode = True
_runner = importlib.import_module("reproduce")
HERE = _runner.HERE
digest = _runner.digest
invoke = _runner.invoke
write = _runner.write
install_handlers = _runner.install_handlers


def verify_audio(run, result):
    """G384-7: independent file-to-oracle check of labels and actual gain wiring.

    Component reconstruction controls cannot detect writing the correct samples
    under the wrong track name or substituting input for the native model output.
    """
    import numpy as np
    from scipy.io import wavfile

    for row in result["rows"]:
        clip = row["id"]
        x = np.fromfile(run / (clip + ".input.f64"), dtype="<f8").reshape(-1, 2)
        g = np.fromfile(run / (clip + ".fitted.f64"), dtype="<f8")
        cv = _runner.decode(run / f"data/cv/p063/{clip}_p063.wav", (len(x),), False)
        hw = _runner.decode(run / f"data/audio_out/p063/{clip}_p063.wav", x.shape, True)
        expected = {
            "01-input": x[:-2],
            "02-model-gain-only": x[:-2] * 10.0 ** (g[:-2, None] / 20.0),
            "03-recorded-hardware": hw[:-2],
            "04-hardware-gain-only": x[:-2] * 10.0 ** (cv[2:, None] / 20.0),
        }
        for name, samples in expected.items():
            for folder, factor in [
                ("measurement", 1.0),
                ("listen", row["audition"]["tracks"][name]["listening_gain"]),
            ]:
                path = run / clip / folder / (name + ".wav")
                rate, actual = wavfile.read(path)
                if rate != 44100 or not np.array_equal(
                    actual, (samples * factor).astype(np.float32)
                ):
                    raise AssertionError("G384-7: labeled audio disagrees with source/gain oracle")


def interrupt_command(out, data):
    """G384-5: actual CLI with a deliberately stalled configure subprocess.

    The external cmake boundary is fault-injected, not numerically qualified
    here. Normal first/recovery runs use real CMake. No production test hook.
    """
    directory = out / "interruption"
    directory.mkdir()
    ready = directory / "ready.json"
    wrapper = directory / "cmake"
    wrapper.write_text(
        "#!/bin/bash\nset -euo pipefail\nexec "
        + shlex.quote(sys.executable)
        + " -c "
        + shlex.quote(
            "import json,os,pathlib,time; "
            f"pathlib.Path({str(ready)!r}).write_text(json.dumps({{'pid':os.getpid()}})); "
            "time.sleep(60)"
        )
        + "\n"
    )
    wrapper.chmod(0o755)
    command = [
        sys.executable,
        str(HERE / "reproduce.py"),
        str(directory / "run"),
        "--data",
        str(data),
    ]
    write(directory / "command.json", command)
    child_pid = None
    with (directory / "owner.log").open("w") as log:
        owner = subprocess.Popen(
            command,
            stdout=log,
            stderr=log,
            start_new_session=True,
            env={**os.environ, "PATH": str(directory) + os.pathsep + os.environ["PATH"]},
        )
        try:
            deadline = time.monotonic() + 60
            while not ready.exists():
                if owner.poll() is not None or time.monotonic() >= deadline:
                    raise AssertionError(
                        "G384-5: configure overlap not observed; inspect owner.log"
                    )
                time.sleep(0.01)
            # Atomic file visibility is not guaranteed; wait for complete JSON.
            while child_pid is None:
                try:
                    child_pid = json.loads(ready.read_text())["pid"]
                except json.JSONDecodeError:
                    if time.monotonic() >= deadline:
                        raise AssertionError("G384-5: incomplete readiness record") from None
                    time.sleep(0.01)
            os.kill(child_pid, 0)
            started = time.monotonic()
            owner.send_signal(signal.SIGINT)
            code = owner.wait(timeout=5)
            elapsed = time.monotonic() - started
            failed = json.loads((directory / "run/results.json").read_text())
            if (
                code != 1
                or failed["verdict"] != "evidence_error"
                or "KeyboardInterrupt" not in failed.get("error", "")
            ):
                raise AssertionError("G384-5: interruption lost failure receipt")
            try:
                os.kill(child_pid, 0)
            except ProcessLookupError:
                pass
            else:
                raise AssertionError("G384-5: configure child survived interruption")
            if elapsed >= 5:
                raise AssertionError("G384-5: cleanup exceeded five seconds")
            write(
                directory / "control.json",
                {
                    "signal": "SIGINT",
                    "child_pid": child_pid,
                    "exit": code,
                    "elapsed_seconds": elapsed,
                    "child_absent": True,
                    "fault": "stalled external configure process",
                },
            )
        finally:
            if owner.poll() is None:
                owner.send_signal(signal.SIGTERM)
                try:
                    owner.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    owner.kill()
                    owner.wait(timeout=5)
            if child_pid is not None:
                try:
                    os.killpg(child_pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass


def qualify(out, data=None):
    if out.exists() or out.is_relative_to(HERE):
        raise ValueError("Choose a fresh external qualification directory")
    out.mkdir(parents=True)
    install_handlers()
    result = {"status": "failed"}

    def execute(name, reuse=None):
        command = [sys.executable, str(HERE / "reproduce.py"), str(out / name)]
        if reuse:
            command += ["--data", str(reuse)]
        code = invoke(command, out, out / (name + ".log"), timeout=3610, allow_failure=True)
        return code, json.loads((out / name / "results.json").read_text())

    def contents(p):
        return {str(f.relative_to(p)): digest(f) for f in p.rglob("*") if f.is_file()}

    try:
        first_code, first = execute("first", data)
        if first_code not in (0, 2):
            raise AssertionError("G384-6: first execution failed")
        verify_audio(out / "first", first)
        # Corrupt only a rendered artifact to prove the file-to-oracle check is
        # sensitive to a plausible, playable but mislabeled model output.
        witness = out / "first/clip_00131/measurement/02-model-gain-only.wav"
        saved = witness.read_bytes()
        witness.write_bytes((witness.parent / "01-input.wav").read_bytes())
        try:
            try:
                verify_audio(out / "first", first)
            except AssertionError:
                pass
            else:
                raise AssertionError("G384-7: substituted dry audio escaped detection")
        finally:
            witness.write_bytes(saved)
        verify_audio(out / "first", first)
        cache = out / "first/data"
        # A wrong byte count exercises acquisition admission through the actual CLI.
        bad = out / "corrupt-data"
        shutil.copytree(cache, bad)
        (bad / "audio_in/clip_00131.wav").write_bytes(b"invalid")
        code, failed = execute("corrupt", bad)
        if (
            code != 1
            or failed["verdict"] != "evidence_error"
            or failed["stage"] != "reference acquisition"
        ):
            raise AssertionError("G384-6: corrupt reference escaped or wrong failure boundary")
        victim = bad / "audio_in/clip_00131.wav"
        original = cache / "audio_in/clip_00131.wav"
        shutil.copyfile(original, victim)
        with victim.open("r+b") as f:
            f.seek(-1, 2)
            value = f.read(1)
            f.seek(-1, 2)
            f.write(bytes([value[0] ^ 1]))
        if victim.stat().st_size != original.stat().st_size or digest(victim) == digest(original):
            raise AssertionError("G384-2: same-size corruption was not activated")
        code, failed = execute("corrupt-hash", bad)
        if (
            code != 1
            or failed["verdict"] != "evidence_error"
            or failed["stage"] != "reference acquisition"
        ):
            raise AssertionError("G384-2: same-size corrupt reference escaped hash admission")
        interrupt_command(out, cache)
        code, recovered = execute("recovered", cache)
        if (
            code != first_code
            or recovered["rows"] != first["rows"]
            or recovered["verdict"] != first["verdict"]
        ):
            raise AssertionError("G384-6: fresh recovery/replay differs")
        gains = {p.name: digest(p) for p in (out / "first").glob("clip_*.f64")}
        if not all(digest(out / "recovered" / n) == h for n, h in gains.items()):
            raise AssertionError("G384-6: gain replay differs")
        audio = {
            str(p.relative_to(out / "first")): digest(p)
            for p in (out / "first").glob("clip_*/*/*.wav")
        }
        if len(audio) != 24 or not all(
            digest(out / "recovered" / n) == h for n, h in audio.items()
        ):
            raise AssertionError("G384-7: missing audio or WAV replay differs")
        before = contents(out / "first")
        code = invoke(
            [sys.executable, str(HERE / "reproduce.py"), str(out / "first")],
            out,
            out / "existing.log",
            allow_failure=True,
        )
        if code == 0 or contents(out / "first") != before:
            raise AssertionError("G384-6: existing output changed")
        result.update(
            status="qualified",
            hardware_verdict=first["verdict"],
            gain_hashes=gains,
            audio_hashes=audio,
            checks=[
                "complete run",
                "corrupt data rejected",
                "same-size corrupt data rejected",
                "actual command interruption and child cleanup",
                "fresh recovery",
                "exact replay",
                "24 audition WAV files replay exactly",
                "labeled audio oracle detects dry substitution; restored files pass",
                "existing output unchanged",
            ],
        )
    except BaseException as exc:
        result["error"] = f"{type(exc).__name__}: {exc}"
        raise
    finally:
        write(out / "qualification.json", result)
    return first_code


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("output", type=Path)
    p.add_argument("--data", type=Path)
    a = p.parse_args()
    sys.exit(qualify(a.output.resolve(), a.data.resolve() if a.data else None))
