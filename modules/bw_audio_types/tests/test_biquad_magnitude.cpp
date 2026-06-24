// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

// =============================================================================
// test_biquad_magnitude - C++ Unit tests
//   (bws::domain::biquadMagnitude + makeFirstOrderHighPass)
// =============================================================================
//
// CONTRACT
// -----------------------------------------------------------------------------
// Source: modules/bw_audio_types/include/bw_audio_types/.h
// biquadMagnitude + makeFirstOrderHighPass.
//
// What the contract requires:
//   - First-order HPF (6 dB/oct): |H| -> 0 at DC, -> 1 at Nyquist, exactly
//     1/sqrt(2) at the corner (bilinear prewarp), strictly monotone-increasing
//     with frequency.
//   - Low/high shelves: |H| equals the set gain at the passband end the shelf
//     acts on and unity at the opposite end; a 0 dB shelf is flat (unity).
//   - The evaluator agrees with a known second-order response (makeLowPass).
// =============================================================================

#include <bw_audio_types/BwsBiquadFilter.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>

using namespace bws::domain;
using Catch::Matchers::WithinAbs;

namespace
{
constexpr double kFs = 48000.0;

float dbOf(float linear)
{
    return 20.0f * std::log10(linear);
}
} // namespace

TEST_CASE("first-order HPF magnitude: DC, Nyquist, corner", "[biquad][magnitude][hpf]")
{
    const double fc = 120.0;
    const auto hp = makeFirstOrderHighPass(kFs, fc);

    // DC is fully rejected; very-low frequency is strongly attenuated.
    CHECK(biquadMagnitude(hp, 1.0, kFs) < 0.05f);
    CHECK(biquadMagnitude(hp, 20.0, kFs) < 0.20f);

    // -3 dB at the corner (1/sqrt(2)), via the bilinear prewarp identity.
    CHECK_THAT(biquadMagnitude(hp, fc, kFs), WithinAbs(0.70710678f, 1e-4f));

    // Passband approaches unity well above the corner.
    CHECK_THAT(biquadMagnitude(hp, 12000.0, kFs), WithinAbs(1.0f, 0.02f));
}

TEST_CASE("first-order HPF magnitude is strictly monotone increasing", "[biquad][magnitude][hpf][monotonic]")
{
    const auto hp = makeFirstOrderHighPass(kFs, 120.0);

    float prev = biquadMagnitude(hp, 10.0, kFs);
    for (double f = 20.0; f <= 20000.0; f *= 1.25)
    {
        const float m = biquadMagnitude(hp, f, kFs);
        CHECK(m > prev);
        prev = m;
    }
}

TEST_CASE("low shelf magnitude: gain at DC, unity at top", "[biquad][magnitude][shelf]")
{
    const float gainLin = std::pow(10.0f, 6.0f / 20.0f); // +6 dB
    const auto ls = makeLowShelf(kFs, 150.0, 0.7071, gainLin);

    // The shelf delivers its set gain in the low band, unity in the high band.
    CHECK_THAT(dbOf(biquadMagnitude(ls, 20.0, kFs)), WithinAbs(6.0f, 0.2f));
    CHECK_THAT(biquadMagnitude(ls, 18000.0, kFs), WithinAbs(1.0f, 0.02f));
}

TEST_CASE("high shelf magnitude: gain at top, unity at DC", "[biquad][magnitude][shelf]")
{
    const float gainLin = std::pow(10.0f, -4.0f / 20.0f); // -4 dB
    const auto hs = makeHighShelf(kFs, 3000.0, 0.7071, gainLin);

    CHECK_THAT(dbOf(biquadMagnitude(hs, 20000.0, kFs)), WithinAbs(-4.0f, 0.3f));
    CHECK_THAT(biquadMagnitude(hs, 20.0, kFs), WithinAbs(1.0f, 0.02f));
}

TEST_CASE("0 dB shelf is flat (bypass identity)", "[biquad][magnitude][shelf][bypass]")
{
    const auto ls = makeLowShelf(kFs, 150.0, 0.7071, 1.0f);
    for (double f = 20.0; f <= 20000.0; f *= 2.0)
    {
        CHECK_THAT(biquadMagnitude(ls, f, kFs), WithinAbs(1.0f, 1e-4f));
    }
}

TEST_CASE("evaluator agrees with a known second-order low-pass", "[biquad][magnitude][lowpass]")
{
    const auto lp = makeLowPass(kFs, 1000.0, 0.7071);

    CHECK_THAT(biquadMagnitude(lp, 20.0, kFs), WithinAbs(1.0f, 0.01f));          // passband
    CHECK_THAT(biquadMagnitude(lp, 1000.0, kFs), WithinAbs(0.70710678f, 0.02f)); // -3 dB corner
    CHECK(biquadMagnitude(lp, 16000.0, kFs) < 0.05f);                            // stopband
}

TEST_CASE("non-positive Q yields finite coefficients", "[biquad][stability]")
{
    const auto finite = [](const BwsBiquadCoeffs& c) {
        return std::isfinite(c.b0) && std::isfinite(c.b1) && std::isfinite(c.b2) && std::isfinite(c.a1) &&
               std::isfinite(c.a2);
    };

    for (const double q : {0.0, -1.0})
    {
        CHECK(finite(makeLowPass(kFs, 1000.0, q)));
        CHECK(finite(makeHighPass(kFs, 1000.0, q)));
        CHECK(finite(makePeak(kFs, 1000.0, q, 2.0f)));
        CHECK(finite(makeLowShelf(kFs, 1000.0, q, 2.0f)));
        CHECK(finite(makeHighShelf(kFs, 1000.0, q, 2.0f)));
    }
}
