// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once

/// @file
/// @brief Transposed Direct Form II biquad filter - a numerically robust float-DSP topology.

// BwsBiquadFilter - Transposed Direct Form II biquad filter.
//
// Industry-standard topology for floating-point audio DSP.
// Numerically superior to standard DF-II: intermediate sums involve
// operands of similar magnitude, reducing float rounding error.
//
// References:
//   - ARM CMSIS-DSP: Transposed DF-II reference algorithm
//   - EarLevel Engineering: "prime candidate for biquad-based equalizers"
//   - Oppenheim & Schafer, "Discrete-Time Signal Processing" (1975)
//
// Coefficient generators use RBJ Audio EQ Cookbook formulas (public domain).
// All derived from analog prototypes via Bilinear Transform with prewarping.
//   - Robert Bristow-Johnson, comp.dsp Usenet (1990s)
//   - W3C Audio EQ Cookbook: https://webaudio.github.io/Audio-EQ-Cookbook/
//
// Zero external dependencies. Pure math. No allocations. RT-safe.

#include <cmath>
#include <numbers>

namespace bws::domain
{

struct BwsBiquadCoeffs
{
    float b0, b1, b2, a1, a2;
};

struct BwsBiquadFilter
{
    BwsBiquadCoeffs coeffs {};
    float d1 {0.0f}, d2 {0.0f}; // Transposed Direct Form II state

    void setCoefficients(const BwsBiquadCoeffs& c) { coeffs = c; }
    void reset()
    {
        d1 = 0.0f;
        d2 = 0.0f;
    }

    void processSamples(float* data, int numSamples) noexcept
    {
        auto c0 = coeffs.b0;
        auto c1 = coeffs.b1;
        auto c2 = coeffs.b2;
        auto c3 = coeffs.a1;
        auto c4 = coeffs.a2;
        auto ld1 = d1, ld2 = d2;

        for (int i = 0; i < numSamples; ++i)
        {
            const float x = data[i];
            const float y = c0 * x + ld1;
            data[i] = y;
            ld1 = c1 * x - c3 * y + ld2;
            ld2 = c2 * x - c4 * y;
        }

        d1 = ld1;
        d2 = ld2;
    }
};

// =============================================================================
// RBJ Audio EQ Cookbook coefficient generators (public domain)
// All use double-precision intermediates, cast to float for storage.
// =============================================================================

namespace detail
{
// Floor keeps sin(w0)/(2*Q) finite at Q <= 0; far below any musical Q.
inline constexpr double kMinFilterQ = 1.0e-4;
inline double clampQ(double q) noexcept
{
    return q > kMinFilterQ ? q : kMinFilterQ;
}
} // namespace detail

inline BwsBiquadCoeffs makeLowPass(double sampleRate, double freq, double Q = 0.7071) noexcept
{
    const double w0 = 2.0 * std::numbers::pi * freq / sampleRate;
    const double cosw0 = std::cos(w0);
    const double alpha = std::sin(w0) / (2.0 * detail::clampQ(Q));
    const double b1 = 1.0 - cosw0;
    const double b0 = b1 * 0.5;
    const double a0 = 1.0 + alpha;
    return {static_cast<float>(b0 / a0), static_cast<float>(b1 / a0), static_cast<float>(b0 / a0),
            static_cast<float>(-2.0 * cosw0 / a0), static_cast<float>((1.0 - alpha) / a0)};
}

inline BwsBiquadCoeffs makeHighPass(double sampleRate, double freq, double Q = 0.7071) noexcept
{
    const double w0 = 2.0 * std::numbers::pi * freq / sampleRate;
    const double cosw0 = std::cos(w0);
    const double alpha = std::sin(w0) / (2.0 * detail::clampQ(Q));
    const double b1 = -(1.0 + cosw0);
    const double b0 = b1 * -0.5;
    const double a0 = 1.0 + alpha;
    return {static_cast<float>(b0 / a0), static_cast<float>(b1 / a0), static_cast<float>(b0 / a0),
            static_cast<float>(-2.0 * cosw0 / a0), static_cast<float>((1.0 - alpha) / a0)};
}

inline BwsBiquadCoeffs makeHighShelf(double sampleRate, double freq, double Q, float gainLinear) noexcept
{
    const double A = std::sqrt(static_cast<double>(gainLinear));
    const double w0 = 2.0 * std::numbers::pi * freq / sampleRate;
    const double cosw0 = std::cos(w0);
    const double sinw0 = std::sin(w0);
    const double alpha = sinw0 / (2.0 * detail::clampQ(Q));
    const double sqrtA = std::sqrt(A);
    const double a0 = (A + 1.0) - (A - 1.0) * cosw0 + 2.0 * sqrtA * alpha;
    const double b0 = A * ((A + 1.0) + (A - 1.0) * cosw0 + 2.0 * sqrtA * alpha);
    const double b1v = -2.0 * A * ((A - 1.0) + (A + 1.0) * cosw0);
    const double b2 = A * ((A + 1.0) + (A - 1.0) * cosw0 - 2.0 * sqrtA * alpha);
    const double a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cosw0);
    const double a2 = (A + 1.0) - (A - 1.0) * cosw0 - 2.0 * sqrtA * alpha;
    return {static_cast<float>(b0 / a0), static_cast<float>(b1v / a0), static_cast<float>(b2 / a0),
            static_cast<float>(a1 / a0), static_cast<float>(a2 / a0)};
}

inline BwsBiquadCoeffs makeLowShelf(double sampleRate, double freq, double Q, float gainLinear) noexcept
{
    const double A = std::sqrt(static_cast<double>(gainLinear));
    const double w0 = 2.0 * std::numbers::pi * freq / sampleRate;
    const double cosw0 = std::cos(w0);
    const double sinw0 = std::sin(w0);
    const double alpha = sinw0 / (2.0 * detail::clampQ(Q));
    const double sqrtA = std::sqrt(A);
    const double a0 = (A + 1.0) + (A - 1.0) * cosw0 + 2.0 * sqrtA * alpha;
    const double b0 = A * ((A + 1.0) - (A - 1.0) * cosw0 + 2.0 * sqrtA * alpha);
    const double b1v = 2.0 * A * ((A - 1.0) - (A + 1.0) * cosw0);
    const double b2 = A * ((A + 1.0) - (A - 1.0) * cosw0 - 2.0 * sqrtA * alpha);
    const double a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cosw0);
    const double a2 = (A + 1.0) + (A - 1.0) * cosw0 - 2.0 * sqrtA * alpha;
    return {static_cast<float>(b0 / a0), static_cast<float>(b1v / a0), static_cast<float>(b2 / a0),
            static_cast<float>(a1 / a0), static_cast<float>(a2 / a0)};
}

// RBJ peaking (bell) EQ. gainLinear = 10^(dB/20) (same convention as the shelves);
// A = sqrt(gainLinear) = 10^(dB/40). gainLinear == 1 (0 dB) returns identity.
inline BwsBiquadCoeffs makePeak(double sampleRate, double freq, double Q, float gainLinear) noexcept
{
    const double A = std::sqrt(static_cast<double>(gainLinear));
    const double w0 = 2.0 * std::numbers::pi * freq / sampleRate;
    const double cosw0 = std::cos(w0);
    const double alpha = std::sin(w0) / (2.0 * detail::clampQ(Q));
    const double a0 = 1.0 + (alpha / A);
    return {static_cast<float>((1.0 + (alpha * A)) / a0), static_cast<float>((-2.0 * cosw0) / a0),
            static_cast<float>((1.0 - (alpha * A)) / a0), static_cast<float>((-2.0 * cosw0) / a0),
            static_cast<float>((1.0 - (alpha / A)) / a0)};
}

// First-order (6 dB/oct) high-pass via bilinear transform with prewarp.
// k = tan(pi fc/fs); H(z) = (1 - z^-1) / ((1+k) + (k-1) z^-1), stored as a
// biquad with b2 = a2 = 0. |H| = 0 at DC, 1 at Nyquist. Matches the detector
// SC-HPF / LF-anchor topology (TptOnePole), not the 2nd-order makeHighPass.
inline BwsBiquadCoeffs makeFirstOrderHighPass(double sampleRate, double freq) noexcept
{
    const double k = std::tan(std::numbers::pi * freq / sampleRate);
    const double norm = 1.0 / (1.0 + k);
    return {static_cast<float>(norm), static_cast<float>(-norm), 0.0f, static_cast<float>((k - 1.0) * norm), 0.0f};
}

// Magnitude response |H(e^jw)| (linear) of a biquad at frequency `freq`, with
// w = 2 pi freq / sampleRate. H(z) = (b0 + b1 z^-1 + b2 z^-2) / (1 + a1 z^-1 + a2 z^-2).
inline float biquadMagnitude(const BwsBiquadCoeffs& c, double freq, double sampleRate) noexcept
{
    const double w = 2.0 * std::numbers::pi * freq / sampleRate;
    const double cosw = std::cos(w);
    const double sinw = std::sin(w);
    const double cos2w = std::cos(2.0 * w);
    const double sin2w = std::sin(2.0 * w);

    const double numRe = c.b0 + (c.b1 * cosw) + (c.b2 * cos2w);
    const double numIm = (c.b1 * sinw) + (c.b2 * sin2w);
    const double denRe = 1.0 + (c.a1 * cosw) + (c.a2 * cos2w);
    const double denIm = (c.a1 * sinw) + (c.a2 * sin2w);

    const double num = std::sqrt((numRe * numRe) + (numIm * numIm));
    const double den = std::sqrt((denRe * denRe) + (denIm * denIm));
    return static_cast<float>(den > 0.0 ? num / den : 0.0);
}

} // namespace bws::domain
