# Copyright (c) 2026 Bellweather Studios.
# SPDX-License-Identifier: Apache-2.0

"""DDM-8: real simulator good/bad/restored/warning and numerical witnesses."""

import os
import re
import tempfile
import unittest
from pathlib import Path

from circuit_process import invoke
from simulator import check_diagnostics

CIRCUIT = """Resistive calibration
V1 in 0 2
R1 in out 1000
R2 out 0 1000
{model}
.control
op
print v(out)
quit
.endc
.end
"""


class SimulatorContract(unittest.TestCase):
    def test_real_tool(self):
        # Exact divider output is an analytic control; removing R2 changes it.
        root = Path(os.environ.get("BWS_CIRCUIT_CONTROLS", tempfile.mkdtemp()))
        root.mkdir(parents=True, exist_ok=True)
        for name, circuit in (
            ("good", CIRCUIT.format(model="")),
            ("bad", CIRCUIT.format(model="").replace("R2 out 0 1000", "R2 out 0 missing")),
            ("restored", CIRCUIT.format(model="")),
            (
                "warning",
                CIRCUIT.format(model=".model detector D(Is=1e-12 nonsense=1)\nDwarn in 0 detector"),
            ),
        ):
            net = root / f"{name}.cir"
            log = root / f"{name}.log"
            net.write_text(circuit)
            code = invoke(["ngspice", "-n", "-b", net.name], root, log, allow_failure=True)
            raw = log.read_text()
            if name == "bad":
                self.assertRegex(raw.lower(), r"error|unknown|missing")
                with self.assertRaises(RuntimeError):
                    check_diagnostics(raw)
                continue
            self.assertEqual(code, 0, raw)
            value = re.search(r"v\(out\)\s*=\s*([\deE.+-]+)", raw)
            self.assertIsNotNone(value, raw)
            self.assertAlmostEqual(float(value.group(1)), 1.0, delta=1e-9)
            if name == "warning":
                self.assertIn("unrecognized parameter (nonsense) - ignored", raw)
                with self.assertRaises(RuntimeError):
                    check_diagnostics(raw)
            else:
                check_diagnostics(raw)
        for invalid in ("", "ngspice-47 done\nWarning: unknown diagnostic"):
            with self.assertRaises(RuntimeError):
                check_diagnostics(invalid)


if __name__ == "__main__":
    unittest.main()
