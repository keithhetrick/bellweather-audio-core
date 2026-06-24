// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
//
// Source: Zavalishin, "The Art of VA Filter Design," Ch. 3 (TPT one-pole).
#include <bw_dsp_core/TptOnePole.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>
#include <limits>
#include <numbers>

using bws::audio::TptOnePole;
using Catch::Matchers::WithinAbs;

TEST_CASE("TptOnePole lowpass passes DC while highpass blocks it", "[tpt][dsp]")
{
    TptOnePole lp;
    lp.prepare(48000.0, 1);
    lp.setType(TptOnePole::Type::Lowpass);
    lp.setCutoffHz(100.0f);

    TptOnePole hp;
    hp.prepare(48000.0, 1);
    hp.setType(TptOnePole::Type::Highpass);
    hp.setCutoffHz(100.0f);

    float lpOut = 0.0f;
    float hpOut = 0.0f;
    for (int n = 0; n < 4000; ++n)
    {
        lpOut = lp.processSample(1.0f);
        hpOut = hp.processSample(1.0f);
    }

    CHECK_THAT(lpOut, WithinAbs(1.0, 1.0e-4));
    CHECK_THAT(hpOut, WithinAbs(0.0, 1.0e-4));
}

TEST_CASE("TptOnePole lowpass and highpass outputs sum to the input", "[tpt][dsp]")
{
    TptOnePole lp;
    lp.prepare(48000.0, 1);
    lp.setType(TptOnePole::Type::Lowpass);
    lp.setCutoffHz(800.0f);

    TptOnePole hp;
    hp.prepare(48000.0, 1);
    hp.setType(TptOnePole::Type::Highpass);
    hp.setCutoffHz(800.0f);

    for (int n = 0; n < 1000; ++n)
    {
        const auto x = static_cast<float>(0.5 * std::sin(2.0 * std::numbers::pi * 440.0 * n / 48000.0));
        const float sum = lp.processSample(x) + hp.processSample(x);
        CHECK_THAT(sum, WithinAbs(x, 1.0e-5));
    }
}

TEST_CASE("TptOnePole clamps cutoff to the stable range", "[tpt][dsp]")
{
    TptOnePole f;
    f.prepare(48000.0, 1);

    f.setCutoffHz(0.0f);
    CHECK_THAT(f.getCutoffHz(), WithinAbs(TptOnePole::kMinCutoffHz, 1.0e-6));

    f.setCutoffHz(1.0e9f);
    const float nyquistGuard = 48000.0f * 0.5f * TptOnePole::kNyquistGuard;
    CHECK(f.getCutoffHz() <= nyquistGuard);
    CHECK(f.getCutoffHz() > 0.0f);
}

TEST_CASE("TptOnePole sanitizes non-finite input", "[tpt][dsp]")
{
    TptOnePole f;
    f.prepare(48000.0, 1);
    f.setType(TptOnePole::Type::Lowpass);

    CHECK(std::isfinite(f.processSample(std::nanf(""))));
    CHECK(std::isfinite(f.processSample(std::numeric_limits<float>::infinity())));
}
