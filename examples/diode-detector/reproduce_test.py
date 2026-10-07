# Copyright (c) 2026 Bellweather Studios.
# SPDX-License-Identifier: Apache-2.0

"""DDM-6: rejected reuse and real missing dependency cannot become success."""

import json
import os
import subprocess
import sys
import tempfile
import unittest
import venv
from pathlib import Path

from reproduce import completed_outcome

HERE = Path(__file__).resolve().parent


class ReproductionContract(unittest.TestCase):
    def test_terminal_verdict_preserves_uncertainty_and_failure(self):
        # DDM-7: execution status and numerical verdict must agree.
        self.assertEqual(completed_outcome(0, "passed"), "passed")
        self.assertEqual(completed_outcome(1, "inconclusive"), "inconclusive")
        for code, reported in (
            (1, "failed"),
            (2, "passed"),
            (-15, "passed"),
            (0, "inconclusive"),
            (0, "missing"),
            (1, "passed"),
        ):
            self.assertEqual(completed_outcome(code, reported), "failed")

    def test_existing_output_is_preserved(self):
        with tempfile.TemporaryDirectory() as directory:
            out = Path(directory) / "existing"
            out.mkdir()
            sentinel = out / "results.json"
            sentinel.write_text("prior evidence\n")
            run = subprocess.run(
                [sys.executable, "-B", str(HERE / "reproduce.py"), str(out)],
                capture_output=True,
                timeout=10,
            )
            self.assertNotEqual(run.returncode, 0)
            self.assertEqual(sentinel.read_text(), "prior evidence\n")
            self.assertIn(b"already exists", run.stderr)

    def test_missing_scientific_dependencies_preserves_failure(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            venv.EnvBuilder(with_pip=False).create(root / "empty-python")
            python = root / "empty-python/bin/python"
            out = root / "failed run"
            env = dict(os.environ)
            env.pop("PYTHONPATH", None)
            run = subprocess.run(
                [str(python), "-B", str(HERE / "reproduce.py"), str(out)],
                capture_output=True,
                env=env,
                timeout=30,
            )
            result = json.loads((out / "results.json").read_text())
            self.assertNotEqual(run.returncode, 0)
            self.assertEqual(result["stage"], "dependencies")
            self.assertEqual(result["outcome"], "failed")
            self.assertIn("numpy", (out / "dependencies.log").read_text())
            self.assertFalse((out / "build").exists())
            self.assertEqual(result["source_sha256"], result["source_after_sha256"])


if __name__ == "__main__":
    unittest.main()
