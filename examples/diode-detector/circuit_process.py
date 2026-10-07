# Copyright (c) 2026 Bellweather Studios.
# SPDX-License-Identifier: Apache-2.0

"""DDM-8: bounded POSIX child ownership for the circuit experiment."""

import os
import signal
import subprocess
import time


def interrupted(signum, _frame):
    raise KeyboardInterrupt(f"signal {signum}")


def install_handlers():
    signal.signal(signal.SIGTERM, interrupted)
    signal.signal(signal.SIGINT, interrupted)


def invoke(command, cwd, log, stdin=None, stdout=None, timeout=180, allow_failure=False):
    with log.open("w") as diagnostics:
        process = subprocess.Popen(
            command,
            cwd=cwd,
            stdin=stdin,
            stdout=stdout or diagnostics,
            stderr=diagnostics,
            start_new_session=True,
        )
        try:
            code = process.wait(timeout=timeout)
        except BaseException:
            deadline = time.monotonic() + 5
            try:
                os.killpg(process.pid, signal.SIGTERM)
            except ProcessLookupError:
                pass
            try:
                process.wait(timeout=2)
            except subprocess.TimeoutExpired:
                pass
            try:
                os.killpg(process.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            process.wait(timeout=max(0.01, deadline - time.monotonic()))
            while time.monotonic() < deadline:
                try:
                    os.killpg(process.pid, 0)
                except ProcessLookupError:
                    break
                time.sleep(0.02)
            else:
                raise RuntimeError(f"DDM-8: process group {process.pid} survived cleanup") from None
            raise
    if code and not allow_failure:
        raise RuntimeError(f"Command failed ({code}): {command}; inspect {log}")
    return code
