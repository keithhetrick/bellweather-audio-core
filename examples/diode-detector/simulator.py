# Copyright (c) 2026 Bellweather Studios.
# SPDX-License-Identifier: Apache-2.0

"""DDM-8: classify the pinned simulator's diagnostics, separately from metrics."""

import re


def check_diagnostics(text):
    if not text.strip() or "ngspice-47" not in text:
        raise RuntimeError("DDM-8: missing ngspice 47 completion witness")
    if re.search(
        r"warning|error|fatal|failed|singular matrix|timestep too small|not converg", text, re.I
    ):
        raise RuntimeError("DDM-8: simulator diagnostic requires investigation")
