// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#include <bw_fft_adapters/BwsFFT.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include <complex>
#include <numbers>
#include <vector>

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;

TEST_CASE("Real FFT forward then inverse round-trip", "[fft][real]")
{
    constexpr int order = 10;
    constexpr int n = 1 << order;
    bws::fft::BwsFFT fft(order);

    std::vector<float> data(n, 0.0f);
    for (int i = 0; i < n; ++i)
        data[i] = std::sin(2.0f * static_cast<float>(std::numbers::pi) * 8.0f * static_cast<float>(i) /
                           static_cast<float>(n));

    std::vector<float> original(data);

    fft.performRealOnlyForwardTransform(data.data());
    fft.performRealOnlyInverseTransform(data.data());

    const float invN = 1.0f / static_cast<float>(n);
    for (int i = 0; i < n; ++i)
        data[i] *= invN;

    for (int i = 0; i < n; ++i)
        REQUIRE_THAT(data[i], WithinAbs(original[i], 1e-4));
}

TEST_CASE("Complex FFT forward then inverse round-trip", "[fft][complex]")
{
    constexpr int order = 10;
    constexpr int n = 1 << order;
    bws::fft::BwsFFT fft(order);

    std::vector<float> data(2 * n, 0.0f);
    data[0] = 1.0f;
    data[1] = 0.0f;
    for (int k = 1; k < n / 2; ++k)
    {
        float mag = 1.0f / static_cast<float>(k + 1);
        float phase = 0.3f * static_cast<float>(k);
        float re = mag * std::cos(phase);
        float im = mag * std::sin(phase);
        data[2 * k] = re;
        data[2 * k + 1] = im;
        int mirror = n - k;
        data[2 * mirror] = re;
        data[2 * mirror + 1] = -im;
    }

    std::vector<float> original(data);

    fft.performComplexInverseTransform(data.data());
    fft.performComplexForwardTransform(data.data());

    const float invN = 1.0f / static_cast<float>(n);
    for (int i = 0; i < 2 * n; ++i)
        data[i] *= invN;

    for (int i = 0; i < 2 * n; ++i)
        REQUIRE_THAT(data[i], WithinAbs(original[i], 1e-3));
}

TEST_CASE("Frequency-only forward produces correct magnitude peak", "[fft][magnitude]")
{
    constexpr int order = 10;
    constexpr int n = 1 << order;
    bws::fft::BwsFFT fft(order);

    constexpr int targetBin = 32;

    std::vector<float> data(n, 0.0f);
    for (int i = 0; i < n; ++i)
        data[i] = std::sin(2.0f * static_cast<float>(std::numbers::pi) * static_cast<float>(targetBin) *
                           static_cast<float>(i) / static_cast<float>(n));

    fft.performFrequencyOnlyForwardTransform(data.data());

    const float expectedMag = static_cast<float>(n) / 2.0f;
    REQUIRE_THAT(data[targetBin], WithinAbs(expectedMag, 1.0));

    for (int k = 1; k < n / 2; ++k)
    {
        if (k == targetBin)
            continue;
        REQUIRE(data[k] < expectedMag * 0.01f);
    }
}

TEST_CASE("DC component: all-ones input", "[fft][dc]")
{
    constexpr int order = 8;
    constexpr int n = 1 << order;
    bws::fft::BwsFFT fft(order);

    std::vector<float> data(n, 1.0f);
    fft.performFrequencyOnlyForwardTransform(data.data());

    REQUIRE_THAT(data[0], WithinAbs(static_cast<double>(n), 0.1));

    for (int k = 1; k <= n / 2; ++k)
        REQUIRE(data[k] < 1.0f);
}

TEST_CASE("Power-of-two edge: order 5 (32 samples)", "[fft][edge]")
{
    constexpr int order = 5;
    constexpr int n = 1 << order;
    bws::fft::BwsFFT fft(order);

    std::vector<float> data(n, 0.0f);
    data[0] = 1.0f;

    std::vector<float> original(data);

    fft.performRealOnlyForwardTransform(data.data());
    fft.performRealOnlyInverseTransform(data.data());

    const float invN = 1.0f / static_cast<float>(n);
    for (int i = 0; i < n; ++i)
        data[i] *= invN;

    for (int i = 0; i < n; ++i)
        REQUIRE_THAT(data[i], WithinAbs(original[i], 1e-5));
}

TEST_CASE("Real FFT conserves energy (Parseval)", "[fft][parseval]")
{
    constexpr int order = 10;
    constexpr int n = 1 << order;
    bws::fft::BwsFFT fft(order);

    const auto twoPiOverN = 2.0f * static_cast<float>(std::numbers::pi) / static_cast<float>(n);
    std::vector<float> x(n, 0.0f);
    for (int i = 0; i < n; ++i)
    {
        const auto t = static_cast<float>(i);
        x[i] = (0.7f * std::sin(twoPiOverN * 5.0f * t)) + (0.3f * std::cos(twoPiOverN * 50.0f * t)) + 0.1f;
    }

    const auto sq = [](float a) {
        return static_cast<double>(a) * static_cast<double>(a);
    };

    double timeEnergy = 0.0;
    for (const float v : x)
    {
        timeEnergy += sq(v);
    }

    std::vector<float> spec(x);
    fft.performFrequencyOnlyForwardTransform(spec.data());

    // One-sided magnitudes double bins 1..N/2-1 for the conjugate-symmetric
    // negative frequencies; Parseval then reads time energy == spectral energy / N.
    double freqEnergy = sq(spec[0]) + sq(spec[n / 2]);
    for (int k = 1; k < n / 2; ++k)
    {
        freqEnergy += 2.0 * sq(spec[k]);
    }
    freqEnergy /= static_cast<double>(n);

    REQUIRE_THAT(freqEnergy, WithinRel(timeEnergy, 1e-3));
}
