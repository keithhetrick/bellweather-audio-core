#!/usr/bin/env python3
# Copyright (c) 2026 Bellweather Studios.
# SPDX-License-Identifier: Apache-2.0

"""Generate EBU Tech 3341 conformance signals as 32-bit float stereo WAVs.

Deterministic from spec formulas - no RNG, no audio-file inputs. Output
goes to the directory passed via --out (default: ./tech3341_out/). Each
case writes <out>/case_N.wav at 48 kHz stereo float32.

EBU Tech 3341 v2.0 §4.1 reference signals (subset relevant to stereo
LUFS conformance):

  Case 1: 1 kHz stereo sine, 20 s, calibrated to -23.0 LUFS integrated
  Case 2: 1 kHz stereo sine, 60 s, calibrated to -33.0 LUFS integrated
  Case 3: 10 s @-36 / 60 s @-23 / 10 s @-36 LUFS - expected integrated -23.0
          LUFS (-36 blocks pass abs gate, fail rel gate)
  Case 4: 10 s @-72 / 10 s @-36 / 60 s @-23 / 10 s @-36 LUFS - expected
          integrated -23.0 LUFS (-72 block dropped by the -70 absolute gate;
          -36 blocks fail the relative gate)

Stereo mono-content (L=R) calibration:
  LUFS = -0.691 + 10·log10(Σ Gi·zi) = -0.691 + 20·log10(A) + 20·log10|H_K(1kHz)|
  So amplitude = 10^((target_lufs + 0.691) / 20) / |H_K(1 kHz)|, where the
  K-weighting gain |H_K(1 kHz)| (~+0.7 dB) is computed from the published 48 kHz
  coefficients (kweighting_magnitude) so the fixture's TRUE loudness = target.
"""

from __future__ import annotations

import argparse
import cmath
import hashlib
import math
import struct
import sys
from pathlib import Path

import numpy as np


SAMPLE_RATE = 48000
NUM_CHANNELS = 2
FREQ_HZ = 1000.0
OVERSAMPLE_HI = 4
SR_HI = SAMPLE_RATE * OVERSAMPLE_HI


# Published ITU-R BS.1770-4/5 48 kHz K-weighting biquad coefficients (Table 1
# spherical-head high-shelf; Table 2 RLB high-pass). Facts stated in the standard
# - used ONLY to calibrate fixture amplitudes so a synthesized tone's TRUE
# K-weighted loudness equals its target. (The K gain at 1 kHz is ~+0.7 dB; the
# earlier "K ~= 0 dB" approximation left fixtures ~0.7 dB hot, so a nominal -23
# fixture actually measured -22.3 on every conformant meter.)
_KWEIGHT_48K = (
    (1.53512485958697, -2.69169618940638, 1.19839281085285, -1.69065929318241, 0.73248077421585),
    (1.0, -2.0, 1.0, -1.99004745483398, 0.99007225036621),
)


def kweighting_magnitude(freq_hz: float, fs: float = SAMPLE_RATE) -> float:
    """|H_K(freq)| of the cascaded 2-stage K-weighting at sample rate fs."""
    w = 2.0 * math.pi * freq_hz / fs
    z1 = cmath.exp(-1j * w)
    z2 = z1 * z1
    mag = 1.0
    for b0, b1, b2, a1, a2 in _KWEIGHT_48K:
        mag *= abs((b0 + b1 * z1 + b2 * z2) / (1.0 + a1 * z1 + a2 * z2))
    return mag


def amplitude_for_lufs(target_lufs: float) -> float:
    """Sine peak amplitude whose TRUE K-weighted integrated loudness equals
    target_lufs for a mono-content stereo 1 kHz signal (L=R, so the two channels'
    energies sum to A^2). Divides out the K-weighting gain at 1 kHz so the fixture
    is genuinely calibrated, not the ~0 dB approximation."""
    return 10.0 ** ((target_lufs + 0.691) / 20.0) / kweighting_magnitude(FREQ_HZ)


def sine_block(seconds: float, amplitude: float) -> np.ndarray:
    """Mono 1 kHz sine of given duration + peak amplitude. Returns float32."""
    n = int(round(seconds * SAMPLE_RATE))
    t = np.arange(n, dtype=np.float64) / SAMPLE_RATE
    return (amplitude * np.sin(2.0 * math.pi * FREQ_HZ * t)).astype(np.float32)


def concat_blocks(*blocks: np.ndarray) -> np.ndarray:
    return np.concatenate(blocks)


def to_stereo(mono: np.ndarray) -> np.ndarray:
    """Interleave L/R = mono/mono → shape (N*2,) float32 ready for WAV write."""
    stereo = np.empty(mono.size * 2, dtype=np.float32)
    stereo[0::2] = mono
    stereo[1::2] = mono
    return stereo


def write_wav_float32(path: Path, interleaved: np.ndarray) -> None:
    """Write a 32-bit float WAV (WAVE_FORMAT_IEEE_FLOAT, code 3).
    `interleaved` is a flat float32 array of [L0, R0, L1, R1, ...]."""
    n_samples_per_ch = interleaved.size // NUM_CHANNELS
    byte_rate = SAMPLE_RATE * NUM_CHANNELS * 4
    block_align = NUM_CHANNELS * 4
    data_bytes = interleaved.tobytes()
    data_len = len(data_bytes)
    fmt_chunk = struct.pack(
        "<4sIHHIIHH",
        b"fmt ", 16, 3, NUM_CHANNELS, SAMPLE_RATE, byte_rate, block_align, 32,
    )
    data_chunk_hdr = struct.pack("<4sI", b"data", data_len)
    riff_size = 4 + len(fmt_chunk) + len(data_chunk_hdr) + data_len
    riff_hdr = struct.pack("<4sI4s", b"RIFF", riff_size, b"WAVE")
    with open(path, "wb") as f:
        f.write(riff_hdr)
        f.write(fmt_chunk)
        f.write(data_chunk_hdr)
        f.write(data_bytes)


def case_1() -> np.ndarray:
    return sine_block(20.0, amplitude_for_lufs(-23.0))


def case_2() -> np.ndarray:
    return sine_block(60.0, amplitude_for_lufs(-33.0))


def case_3() -> np.ndarray:
    # EBU Tech 3341 case 3: 10 s @-36 / 60 s @-23 / 10 s @-36. The -36 blocks
    # pass the -70 absolute gate but fail the relative gate (the 60 s @-23 lifts
    # the gated mean so the rel gate rises above -36) -> integrated = -23.0.
    a_quiet = amplitude_for_lufs(-36.0)
    a_loud = amplitude_for_lufs(-23.0)
    return concat_blocks(
        sine_block(10.0, a_quiet),
        sine_block(60.0, a_loud),
        sine_block(10.0, a_quiet),
    )


def case_4() -> np.ndarray:
    # EBU Tech 3341 case 4: 10 s @-72 / 10 s @-36 / 60 s @-23 / 10 s @-36. The
    # -72 block is below the -70 ABSOLUTE gate (dropped); the -36 blocks fail the
    # relative gate -> integrated = -23.0. Exercises the absolute gate.
    a_silent = amplitude_for_lufs(-72.0)
    a_quiet = amplitude_for_lufs(-36.0)
    a_loud = amplitude_for_lufs(-23.0)
    return concat_blocks(
        sine_block(10.0, a_silent),
        sine_block(10.0, a_quiet),
        sine_block(60.0, a_loud),
        sine_block(10.0, a_quiet),
    )


def _fs4_burst_in_fs6_hi(seconds: float) -> np.ndarray:
    """At 4·fs: an fs/6 sine (amp 0.50) with one period of an fs/4 sine
    (amp 1.00) replacing a window at a host zero-up crossing (value-continuous
    splice; the anti-alias filter smooths the residual slope step)."""
    n = int(round(seconds * SR_HI))
    t = np.arange(n, dtype=np.float64) / SR_HI
    fs6 = SAMPLE_RATE / 6.0
    fs4 = SAMPLE_RATE / 4.0
    host = 0.50 * np.sin(2.0 * math.pi * fs6 * t)
    period = int(round(SR_HI / fs4))
    host_period = SR_HI / fs6
    start = int(round(math.ceil((n // 2) / host_period) * host_period))
    burst_t = np.arange(period, dtype=np.float64) / SR_HI
    burst = 1.00 * np.sin(2.0 * math.pi * fs4 * burst_t)
    sig = host.copy()
    sig[start:start + period] = burst
    return sig


def _lowpass_taps(numtaps: int, cutoff_hz: float, fs: float, window) -> np.ndarray:
    """Windowed-sinc lowpass, unity DC gain. fc as a fraction of fs."""
    fc = cutoff_hz / fs
    n = np.arange(numtaps, dtype=np.float64) - (numtaps - 1) / 2.0
    h = 2.0 * fc * np.sinc(2.0 * fc * n) * window(numtaps)
    return h / np.sum(h)


def _anti_alias_decimate(sig_hi: np.ndarray, offset: int) -> np.ndarray:
    """Lowpass below the 48 kHz Nyquist, group-delay-compensate, then keep every
    4th sample at the given 4·fs-rate offset → 48 kHz mono float32."""
    taps = _lowpass_taps(193, SAMPLE_RATE / 2.0, SR_HI, np.hamming)
    filtered = np.convolve(sig_hi, taps, mode="full")[: sig_hi.size]
    delay = (len(taps) - 1) // 2
    return filtered[delay + offset::OVERSAMPLE_HI].astype(np.float32)


def _embedded_isp_case(offset: int) -> np.ndarray:
    return _anti_alias_decimate(_fs4_burst_in_fs6_hi(1.0), offset)


def case_20() -> np.ndarray:
    return _embedded_isp_case(0)


def case_21() -> np.ndarray:
    return _embedded_isp_case(1)


def case_22() -> np.ndarray:
    return _embedded_isp_case(2)


def case_23() -> np.ndarray:
    return _embedded_isp_case(3)


def independent_true_peak_db(mono: np.ndarray, oversample: int = 4) -> float:
    """True peak via zero-stuff + Blackman-windowed-sinc interpolation - a
    distinct window from both the Hamming synthesis filter and the C++ Hann-sinc
    meter, so agreement across the three is meaningful."""
    up = np.zeros(mono.size * oversample, dtype=np.float64)
    up[::oversample] = mono.astype(np.float64)
    taps = _lowpass_taps(193, SAMPLE_RATE / 2.0, SAMPLE_RATE * oversample, np.blackman) * oversample
    filt = np.convolve(up, taps, mode="same")
    peak = float(np.max(np.abs(filt)))
    return 20.0 * math.log10(peak) if peak > 0.0 else -120.0


CASES = {
    "case_1.wav": case_1,
    "case_2.wav": case_2,
    "case_3.wav": case_3,
    "case_4.wav": case_4,
    "case_20.wav": case_20,
    "case_21.wav": case_21,
    "case_22.wav": case_22,
    "case_23.wav": case_23,
}

# True-peak fixtures whose independent-oracle dBTP must match the EBU value
# before they are trusted as test inputs.
VERIFY_TRUE_PEAK_DBTP = {
    "case_20.wav": 0.0,
    "case_21.wav": 0.0,
    "case_22.wav": 0.0,
    "case_23.wav": 0.0,
}


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 16), b""):
            h.update(chunk)
    return h.hexdigest()


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "--out",
        type=Path,
        default=Path("tech3341_out"),
        help="Output directory (will be created if missing).",
    )
    ap.add_argument(
        "--print-sha256",
        action="store_true",
        help="Print SHA256 of each generated file to stdout.",
    )
    ap.add_argument(
        "--verify",
        action="store_true",
        help="Check each true-peak fixture against the independent oracle and "
        "exit non-zero if any deviates from its EBU dBTP value.",
    )
    args = ap.parse_args(argv)

    if args.verify:
        ok = True
        for name, expected in VERIFY_TRUE_PEAK_DBTP.items():
            measured = independent_true_peak_db(CASES[name]())
            within = abs(measured - expected) <= 0.1
            ok = ok and within
            mark = "OK" if within else "FAIL"
            print(f"[{mark}] {name}: true-peak {measured:+.4f} dBTP (expected {expected:+.1f})")
        return 0 if ok else 1

    args.out.mkdir(parents=True, exist_ok=True)

    for name, generator in CASES.items():
        mono = generator()
        stereo = to_stereo(mono)
        path = args.out / name
        write_wav_float32(path, stereo)
        if args.print_sha256:
            print(f"{sha256(path)}  {name}")
        else:
            print(f"wrote {path} ({stereo.size * 4} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
