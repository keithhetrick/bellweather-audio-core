# Copyright (c) 2026 Bellweather Studios.
# SPDX-License-Identifier: Apache-2.0

"""Acceptance checks from diode-detector-modeling.md; no implementation oracle."""

import numpy as np


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def errors(actual, reference):
    require(actual.shape == reference.shape and actual.size > 0, "DDM-3: shape mismatch")
    require(np.isfinite(actual).all() and np.isfinite(reference).all(), "DDM-3: nonfinite data")
    delta = actual - reference
    return {"max_V": float(np.max(np.abs(delta))), "rms_V": float(np.sqrt(np.mean(delta * delta)))}


def check_port(actual, reference):
    measured = errors(actual, reference)
    require(measured["max_V"] <= 1e-9, "DDM-3: native/Python disagreement")
    return measured


def check_dc(observed, expected):
    require(abs(observed - expected) <= 1e-5, "DDM-3: independent DC mismatch")


def check_source(actual, reference):
    measured = errors(actual, reference)
    require(measured["max_V"] <= 1e-6, "DDM-4: source mismatch")
    return measured


def check_convergence(base_error, fine_error, reference_disagreement=0.0):
    require(fine_error < base_error, "DDM-4: 4x did not improve on 1x")
    return base_error - fine_error > 2 * reference_disagreement


def check_negative(correct_error, wrong_error):
    require(
        wrong_error >= 2 * correct_error and wrong_error > 0, "DDM-4: wrong-C control not detected"
    )
