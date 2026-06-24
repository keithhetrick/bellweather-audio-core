// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

// Contract: modules/bw_dsp_core/include/bw_dsp_core/DspConstants.h -
// time and gain/decibel conversion helpers.

#include <bw_dsp_core/DspConstants.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <limits>

using bws::dsp::DspMath;
using Catch::Approx;

namespace
{
constexpr double kFs = 48000.0;
} // namespace

TEST_CASE("msToSamples converts and round-trips with samplesToMs", "[dsp_constants][time]")
{
    CHECK(DspMath::msToSamples(0.0, kFs) == 0);
    CHECK(DspMath::msToSamples(10.0, kFs) == 480);
    CHECK(DspMath::msToSamples(1000.0, kFs) == 48000);

    const int n = DspMath::msToSamples(25.0, kFs);
    CHECK(DspMath::samplesToMs(n, kFs) == Approx(25.0).margin(0.05));
}

TEST_CASE("msToSamples clamps out-of-range input to int bounds", "[dsp_constants][time][stability]")
{
    CHECK(DspMath::msToSamples(1.0e18, kFs) == std::numeric_limits<int>::max());
    CHECK(DspMath::msToSamples(-1.0e18, kFs) == std::numeric_limits<int>::min());
}

TEST_CASE("gain and decibel conversions round-trip and floor at silence", "[dsp_constants][gain]")
{
    CHECK(DspMath::dbToGain(0.0f) == Approx(1.0f));
    CHECK(DspMath::gainToDb(1.0f) == Approx(0.0f).margin(1e-4));
    CHECK(DspMath::dbToGain(-6.0f) == Approx(0.50118f).margin(1e-4));

    const float g = 0.25f;
    CHECK(DspMath::dbToGain(DspMath::gainToDb(g)) == Approx(g).margin(1e-4));
    CHECK(std::isfinite(DspMath::gainToDb(0.0f)));
}
