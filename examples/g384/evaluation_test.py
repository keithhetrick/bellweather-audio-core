# Copyright (c) 2026 Bellweather Studios.
# SPDX-License-Identifier: Apache-2.0

"""G384-3 component oracles: metric and complete-record verdict detection."""

import math
import unittest

import numpy as np
from reproduce import compare, listening_gains, metrics, reconstruct, verdict


class AuditionTests(unittest.TestCase):
    def test_gain_units_channels_and_fixed_offset(self):
        # G384-7: independent unity/tenth-amplitude controls expose dB scaling,
        # stereo broadcasting and offset errors before real native integration.
        x = np.tile([1.0, -1.0], (5, 1))
        model, hardware = reconstruct(x, np.zeros(5), np.array([40.0, 40.0, -20.0, -20.0, -20.0]))
        np.testing.assert_array_equal(model, x[:3])
        np.testing.assert_allclose(hardware, x[:3] / 10, rtol=0, atol=1e-15)
        model, _ = reconstruct(x, np.full(5, -20.0), np.zeros(5))
        np.testing.assert_allclose(model, x[:3] / 10, rtol=0, atol=1e-15)

    def test_matching_ceiling_and_invalid_inputs(self):
        # G384-7: full/half-scale oracle; witnesses reject silence/NaN/framing.
        x = np.tile([1.0, -1.0], (5, 1))
        np.testing.assert_array_equal(listening_gains([x, x / 2]), [0.5, 1.0])
        self.assertAlmostEqual(listening_gains([x])[0], 10 ** (-3 / 20), delta=1e-15)
        for tracks in [[], [x, x[:2]], [x * 0], [x * float("nan")]]:
            with self.assertRaises(ValueError):
                listening_gains(tracks)
        for gain in [np.zeros(4), np.full(5, float("nan")), np.zeros((5, 1))]:
            with self.assertRaises(ValueError):
                reconstruct(x, gain, np.zeros(5))


class ContractTests(unittest.TestCase):
    def test_alignment_zero_and_one_sample_errors(self):
        # G384-3: independent ramp oracle. Scalar metrics cannot detect a
        # reversed/double-applied lag in the actual record comparison.
        g = np.arange(44103, dtype=float)
        cv = np.arange(-2, 44101, dtype=float)
        total, windows = compare(g, cv)
        self.assertTrue(all(v == 0 for v in total.values()))
        self.assertEqual(
            [(w["start_frame"], w["frames"]) for w in windows], [(0, 44100), (44100, 1)]
        )
        self.assertTrue(all(w["peak_dB"] == 0 for w in windows))
        for shift in [-1, 1]:
            total, windows = compare(g, cv + shift)
            self.assertEqual(total["mae_dB"], 1)
            self.assertEqual(total["bias_dB"], -shift)
            self.assertTrue(all(w["rms_dB"] == 1 for w in windows))

    def test_window_boundary_and_partial_tail(self):
        # G384-3: impulses in adjacent windows detect omission, overlap and
        # warmup removal without using the comparison to generate its oracle.
        g = np.zeros(44103)
        g[44099], g[44100] = 3, -4
        total, windows = compare(g, np.zeros(44103))
        self.assertEqual(
            [(w["start_frame"], w["frames"]) for w in windows], [(0, 44100), (44100, 1)]
        )
        self.assertAlmostEqual(total["mae_dB"], 7 / 44101, delta=1e-12)
        self.assertAlmostEqual(total["rms_dB"], math.sqrt(25 / 44101), delta=1e-12)
        self.assertAlmostEqual(windows[0]["mae_dB"], 3 / 44100, delta=1e-12)
        self.assertEqual(windows[1]["bias_dB"], -4)
        self.assertEqual(windows[1]["peak_dB"], 4)
        g[:] = 0
        g[0] = 6
        total, _ = compare(g, np.zeros(44103))
        self.assertAlmostEqual(total["mae_dB"], 6 / 44101, delta=1e-12)

    def test_invalid_complete_records(self):
        for a, b in [
            (np.zeros(2), np.zeros(2)),
            (np.zeros(4), np.zeros(5)),
            (np.zeros((3, 2)), np.zeros((3, 2))),
            (np.array([0.0, 0.0, float("nan")]), np.zeros(3)),
        ]:
            with self.assertRaises(ValueError):
                compare(a, b)

    def test_zero_span_offset_sign(self):
        self.assertTrue(all(v == 0 for v in metrics(np.zeros(2), np.zeros(2)).values()))
        actual = metrics(np.array([3.0, -4.0]), np.zeros(2))
        for key, expected in dict(
            mae_dB=3.5, rms_dB=math.sqrt(12.5), peak_dB=4.0, bias_dB=-0.5
        ).items():
            self.assertAlmostEqual(actual[key], expected, delta=1e-12)
        self.assertEqual(metrics(np.array([6.0]), np.array([0.0]))["mae_dB"], 6)
        self.assertEqual(metrics(np.array([6.0]), np.array([-6.0]))["mae_dB"], 12)

    def test_invalid_measurements(self):
        for a, b in [
            (np.array([]), np.array([])),
            (np.zeros(2), np.zeros(3)),
            (np.array([float("nan")]), np.zeros(1)),
            (np.zeros(1), np.array([float("inf")])),
            (np.zeros((1, 2)), np.zeros((1, 2))),
        ]:
            with self.assertRaises(ValueError):
                metrics(a, b)

    def records(self):
        return [
            {
                "id": c,
                "nominal": {"mae_dB": 2.0, "rms_dB": 3.0},
                "fitted": {"mae_dB": 1.0, "rms_dB": 2.0},
            }
            for c in ["clip_00131", "clip_00143", "clip_00209"]
        ]

    def test_complete_improvement(self):
        self.assertEqual(verdict(self.records()), "improved_on_all_records")

    def test_adverse_and_equal_single_record(self):
        for key in ["mae_dB", "rms_dB"]:
            for increase in [0.0, 1.0]:
                rows = self.records()
                rows[1]["fitted"][key] = rows[1]["nominal"][key] + increase
                self.assertEqual(verdict(rows), "mixed_or_not_improved")

    def test_missing_duplicate_and_integrity_precedence(self):
        self.assertEqual(verdict(self.records()[:2]), "evidence_error")
        rows = self.records()
        rows[2]["id"] = rows[0]["id"]
        self.assertEqual(verdict(rows), "evidence_error")
        self.assertEqual(verdict(self.records(), integrity=False), "evidence_error")
        rows = self.records()
        rows[0]["fitted"]["mae_dB"] = float("nan")
        self.assertEqual(verdict(rows), "evidence_error")


class DecodeTests(unittest.TestCase):
    def test_pcm_scale_cv_units_and_wrong_format(self):
        import struct
        import tempfile
        from pathlib import Path

        from reproduce import decode

        def wave(path, kind, bits, body, rate=44100):
            fmt = struct.pack("<HHIIHH", kind, 1, rate, rate * bits // 8, bits // 8, bits)
            payload = (
                b"WAVEfmt "
                + struct.pack("<I", 16)
                + fmt
                + b"data"
                + struct.pack("<I", len(body))
                + body
            )
            if len(body) % 2:
                payload += bytes(1)
            path.write_bytes(b"RIFF" + struct.pack("<I", len(payload)) + payload)

        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / "test.wav"
            wave(
                p,
                1,
                24,
                b"".join(v.to_bytes(3, "little", signed=True) for v in [-8388608, 0, 4194304]),
            )
            self.assertTrue(np.array_equal(decode(p, (3,), True), [-1, 0, 0.5]))
            with self.assertRaises(ValueError):
                decode(p, (3,), False)
            wave(p, 3, 32, struct.pack("<fff", -6, 0, 0.25))
            self.assertTrue(np.array_equal(decode(p, (3,), False), [-6, 0, 0.25]))
            with self.assertRaises(ValueError):
                decode(p, (3,), True)
            wave(p, 3, 32, struct.pack("<fff", -6, 0, 0.25), 48000)
            with self.assertRaises(ValueError):
                decode(p, (3,), False)


if __name__ == "__main__":
    unittest.main()
