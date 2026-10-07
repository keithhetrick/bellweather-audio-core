#!/usr/bin/env python3
# Copyright (c) 2026 Bellweather Studios.
# SPDX-License-Identifier: Apache-2.0

"""DDM-3/4 metrology zero/span and deliberately invalid evidence witnesses."""

import unittest

import numpy as np
from checks import check_convergence, check_dc, check_negative, check_port, check_source, errors


class MeasurementContract(unittest.TestCase):
    def test_zero_and_span(self):
        zero = np.zeros(4)
        self.assertEqual(errors(zero, zero), {"max_V": 0.0, "rms_V": 0.0})
        for metric in errors(zero + 0.01, zero).values():
            self.assertAlmostEqual(metric, 0.01, delta=1e-12)

    def test_resolved_improvement(self):
        # DDM-7: synthetic RMS and reference disagreement, not model outputs.
        self.assertTrue(check_convergence(0.01, 0.001, 0.001))
        self.assertFalse(check_convergence(0.01, 0.009, 0.001))
        self.assertFalse(check_convergence(0.5, 0.25, 0.125))
        for base, fine in ((0.01, 0.01), (0.01, 0.02)):
            with self.assertRaises(RuntimeError):
                check_convergence(base, fine, 0.001)

    def test_invalid_evidence(self):
        for invalid in (np.array([np.nan]), np.array([np.inf]), np.zeros(2), np.zeros(0)):
            with self.assertRaises(RuntimeError):
                errors(invalid, np.zeros(1))

    def test_acceptance_reaches_failure(self):
        check_source(np.zeros(2), np.zeros(2))
        with self.assertRaises(RuntimeError):
            check_source(np.ones(2) * 0.01, np.zeros(2))
        check_port(np.zeros(2), np.zeros(2))
        with self.assertRaises(RuntimeError):
            check_port(np.ones(2) * 1e-5, np.zeros(2))
        check_dc(1.0, 1.0)
        with self.assertRaises(RuntimeError):
            check_dc(1.001, 1.0)
        check_convergence(0.01, 0.001)
        with self.assertRaises(RuntimeError):
            check_convergence(0.01, 0.02)
        check_negative(0.01, 0.1)
        with self.assertRaises(RuntimeError):
            check_negative(0.01, 0.01)


if __name__ == "__main__":
    unittest.main()
