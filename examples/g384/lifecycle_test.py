# Copyright (c) 2026 Bellweather Studios.
# SPDX-License-Identifier: Apache-2.0

"""G384-5: real child lifetime across success, timeout and orchestrator interrupt."""

import os
import signal
import subprocess
import sys
import tempfile
import time
import unittest
from pathlib import Path

from reproduce import invoke

HERE = Path(__file__).resolve().parent
CHILD = "import os,time; print(os.getpid(),flush=True); time.sleep(30)"

TREE = """import os,signal,subprocess,sys,time
child=subprocess.Popen([sys.executable,'-c','import time; time.sleep(30)'])
def stop(*_):
    child.wait(timeout=3)
    sys.exit(0)
signal.signal(signal.SIGTERM,stop)
print(os.getpid(),child.pid,flush=True)
time.sleep(30)
"""


class ProcessCustodyContract(unittest.TestCase):
    def assert_absent(self, pid):
        with self.assertRaises(ProcessLookupError):
            os.kill(pid, 0)

    def test_success(self):
        with tempfile.TemporaryDirectory() as directory:
            log = Path(directory) / "success.log"
            invoke([sys.executable, "-c", "print('completed')"], HERE, log, timeout=5)
            self.assertEqual(log.read_text().strip(), "completed")

    def test_timeout(self):
        with tempfile.TemporaryDirectory() as directory:
            log = Path(directory) / "timeout.log"
            started = time.monotonic()
            with self.assertRaises(subprocess.TimeoutExpired):
                invoke([sys.executable, "-c", CHILD], HERE, log, timeout=1)
            self.assert_absent(int(log.read_text().strip()))
            self.assertLess(time.monotonic() - started, 6.0)

    def test_interrupt(self):
        self.check_interrupt(signal.SIGINT)

    def test_terminate(self):
        self.check_interrupt(signal.SIGTERM)

    def test_timeout_descendant(self):
        with tempfile.TemporaryDirectory() as directory:
            log = Path(directory) / "tree.log"
            started = time.monotonic()
            with self.assertRaises(subprocess.TimeoutExpired):
                invoke([sys.executable, "-c", TREE], HERE, log, timeout=1)
            for pid in map(int, log.read_text().split()):
                self.assert_absent(pid)
            self.assertLess(time.monotonic() - started, 6.0)

    def test_interrupt_descendant(self):
        self.check_interrupt(signal.SIGINT, TREE)

    def check_interrupt(self, signum, child=CHILD):
        with tempfile.TemporaryDirectory() as directory:
            log = Path(directory) / "child.log"
            script = (
                "import sys; from pathlib import Path; from reproduce import invoke, install_handlers; install_handlers()\n"
                "try:\n"
                " invoke([sys.executable,'-c',sys.argv[2]],Path.cwd(),Path(sys.argv[1]),timeout=20)\n"
                "except KeyboardInterrupt:\n"
                " sys.exit(73)\n"
            )
            owner = subprocess.Popen(
                [sys.executable, "-c", script, str(log), child],
                cwd=HERE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                start_new_session=True,
            )
            child_pid = None
            try:
                deadline = time.monotonic() + 10
                while time.monotonic() < deadline:
                    if log.exists() and log.read_text().strip():
                        child_pid = int(log.read_text().split()[0])
                        break
                    self.assertIsNone(owner.poll(), "Orchestrator exited before child readiness")
                    time.sleep(0.02)
                self.assertIsNotNone(child_pid, "Child readiness deadline exceeded")
                started = time.monotonic()
                owner.send_signal(signum)
                _, diagnostics = owner.communicate(timeout=10)
                self.assertEqual(owner.returncode, 73, diagnostics.decode())
                for pid in map(int, log.read_text().split()):
                    self.assert_absent(pid)
                self.assertLess(time.monotonic() - started, 5.0)
            finally:
                if owner.poll() is None:
                    owner.kill()
                    owner.communicate(timeout=5)
                if child_pid is not None:
                    try:
                        os.killpg(child_pid, signal.SIGKILL)
                    except ProcessLookupError:
                        pass


if __name__ == "__main__":
    unittest.main()
