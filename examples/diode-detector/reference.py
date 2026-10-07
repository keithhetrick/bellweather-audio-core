#!/usr/bin/env python3
# Copyright (c) 2026 Bellweather Studios.
# SPDX-License-Identifier: Apache-2.0

"""Offline backward-Euler reference, DDM-1/3. SciPy root solving, not production DSP."""

import json
import math

import numpy as np
from checks import check_dc, require
from scipy.optimize import brentq

VT = 8.617333262145e-5 * 300.0


def render(samples, rate, capacitance=1e-6):
    a = capacitance * rate + 1 / 100000
    resistance = 1000 + 1 / a
    v = 0.0
    result = np.empty(len(samples))
    for index, u in enumerate(samples):
        history = capacitance * rate * v
        offset = u - history / a
        if offset == 0:
            diode = 0.0
        else:
            diode = brentq(
                lambda d, offset=offset: d + resistance * 1e-12 * math.expm1(d / VT) - offset,
                min(offset, 0),
                max(offset, 0),
                xtol=5e-15,
            )
        v = ((offset - diode) / resistance + history) / a
        result[index] = v
    return result


def dc_voltage(source=2.0):
    return brentq(
        lambda v: source - v - 1000 * v / 100000 - VT * math.log1p(v / (100000 * 1e-12)),
        0.0,
        source,
        xtol=5e-15,
    )


if __name__ == "__main__":
    zero = render(np.zeros(100), 48000)
    require(np.count_nonzero(zero) == 0, "DDM-3: zero reference failed")
    observed = render(np.full(24000, 2.0), 48000)[-1]
    expected = dc_voltage()
    check_dc(observed, expected)
    print(
        json.dumps(
            {
                "outcome": "passed",
                "zero_max_V": float(np.max(np.abs(zero))),
                "DC_expected_V": expected,
                "DC_observed_V": float(observed),
            }
        )
    )
