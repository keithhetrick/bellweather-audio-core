// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
// Public contract tests for reusable stereo utility processors.

#include "bw_dsp_core/PhaseInverter.h"
#include "bw_dsp_processors/ChannelSolo.h"
#include "bw_dsp_processors/ChannelSwap.h"
#include "bw_dsp_processors/MonoSum.h"
#include "bw_dsp_processors/StereoBalancer.h"
#include "bw_dsp_processors/StereoWidth.h"

#include <bw_audio_types/BufferView.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <array>
#include <vector>

using bws::dsp::ChannelSolo;
using bws::dsp::ChannelSwap;
using bws::dsp::MonoSum;
using bws::dsp::PhaseInverter;
using bws::dsp::StereoBalancer;
using bws::dsp::StereoWidth;
using Catch::Approx;

namespace
{
constexpr double kSampleRate = 48000.0;

// Run a processor over an in-place stereo pair.
template <typename Proc>
void run(Proc& proc, std::vector<float>& left, std::vector<float>& right)
{
    std::array<float*, 2> chans {left.data(), right.data()};
    proc.process(bws::domain::BufferView {chans.data(), 2, static_cast<int>(left.size())});
}
} // namespace

TEST_CASE("PhaseInverter: enabling left inverts left only", "[utility][phase_inverter][gate]")
{
    PhaseInverter p;
    p.prepare(kSampleRate, 64);
    p.setInvertLeft(true);

    std::vector<float> L {0.5f, -0.3f, 0.7f, 1.0f};
    std::vector<float> R {0.2f, 0.4f, -0.6f, -1.0f};
    const auto L0 = L, R0 = R;
    run(p, L, R);

    for (size_t i = 0; i < L.size(); ++i)
    {
        REQUIRE(L[i] == Approx(-L0[i]));
        REQUIRE(R[i] == Approx(R0[i]));
    }
}

TEST_CASE("PhaseInverter: double invert is identity", "[utility][phase_inverter][gate]")
{
    PhaseInverter p;
    p.prepare(kSampleRate, 64);
    p.setInvertLeft(true);
    p.setInvertRight(true);

    std::vector<float> L {0.5f, -0.3f, 0.7f};
    std::vector<float> R {0.2f, 0.4f, -0.6f};
    const auto L0 = L, R0 = R;
    run(p, L, R);
    run(p, L, R);

    for (size_t i = 0; i < L.size(); ++i)
    {
        REQUIRE(L[i] == Approx(L0[i]));
        REQUIRE(R[i] == Approx(R0[i]));
    }
}

TEST_CASE("PhaseInverter: disabled is bit-exact passthrough", "[utility][phase_inverter][gate]")
{
    PhaseInverter p;
    p.prepare(kSampleRate, 64);

    std::vector<float> L {0.5f, -0.3f, 0.7f};
    std::vector<float> R {0.2f, 0.4f, -0.6f};
    const auto L0 = L, R0 = R;
    run(p, L, R);

    REQUIRE(L == L0);
    REQUIRE(R == R0);
}

TEST_CASE("MonoSum: enabled folds to (L+R)/2 on both channels", "[utility][mono_sum][gate]")
{
    MonoSum m;
    m.prepare(kSampleRate, 64);
    m.setEnabled(true);

    std::vector<float> L {1.0f, 0.5f, -0.2f, 0.8f};
    std::vector<float> R {0.0f, -0.5f, 0.6f, 0.8f};
    const auto L0 = L, R0 = R;
    run(m, L, R);

    for (size_t i = 0; i < L.size(); ++i)
    {
        const float mono = (L0[i] + R0[i]) * 0.5f;
        REQUIRE(L[i] == Approx(mono));
        REQUIRE(R[i] == Approx(mono));
    }
}

TEST_CASE("MonoSum: correlated (mono) input passes at unity", "[utility][mono_sum][gate]")
{
    MonoSum m;
    m.prepare(kSampleRate, 64);
    m.setEnabled(true);

    std::vector<float> L {0.3f, -0.7f, 0.9f};
    std::vector<float> R = L;
    const auto L0 = L;
    run(m, L, R);

    for (size_t i = 0; i < L.size(); ++i)
    {
        REQUIRE(L[i] == Approx(L0[i]));
        REQUIRE(R[i] == Approx(L0[i]));
    }
}

TEST_CASE("MonoSum: anti-phase input cancels to silence", "[utility][mono_sum][gate]")
{
    MonoSum m;
    m.prepare(kSampleRate, 64);
    m.setEnabled(true);

    std::vector<float> L {0.5f, -0.3f, 0.9f};
    std::vector<float> R {-0.5f, 0.3f, -0.9f};
    run(m, L, R);

    for (size_t i = 0; i < L.size(); ++i)
    {
        REQUIRE(L[i] == Approx(0.0f).margin(1e-7));
        REQUIRE(R[i] == Approx(0.0f).margin(1e-7));
    }
}

TEST_CASE("MonoSum: disabled is passthrough", "[utility][mono_sum][gate]")
{
    MonoSum m;
    m.prepare(kSampleRate, 64);

    std::vector<float> L {0.5f, -0.3f}, R {0.2f, 0.4f};
    const auto L0 = L, R0 = R;
    run(m, L, R);

    REQUIRE(L == L0);
    REQUIRE(R == R0);
}

namespace
{
// Settle the smoother on a constant input, return the final-sample gains.
std::pair<float, float> settledBalanceGains(float balance)
{
    StereoBalancer b;
    b.prepare(kSampleRate, 512);
    b.setBalance(balance);
    std::vector<float> L, R;
    for (int block = 0; block < 8; ++block)
    {
        L.assign(512, 1.0f);
        R.assign(512, 1.0f);
        run(b, L, R);
    }
    return {L.back(), R.back()};
}
} // namespace

TEST_CASE("StereoBalancer: center is unity on both channels", "[utility][balance][gate]")
{
    const auto [gl, gr] = settledBalanceGains(0.0f);
    REQUIRE(gl == Approx(1.0f).margin(1e-3));
    REQUIRE(gr == Approx(1.0f).margin(1e-3));
}

TEST_CASE("StereoBalancer: full-left mutes right, left at unity", "[utility][balance][gate]")
{
    const auto [gl, gr] = settledBalanceGains(-1.0f);
    REQUIRE(gl == Approx(1.0f).margin(1e-3));
    REQUIRE(gr == Approx(0.0f).margin(1e-3));
}

TEST_CASE("StereoBalancer: full-right mutes left, right at unity", "[utility][balance][gate]")
{
    const auto [gl, gr] = settledBalanceGains(1.0f);
    REQUIRE(gl == Approx(0.0f).margin(1e-3));
    REQUIRE(gr == Approx(1.0f).margin(1e-3));
}

TEST_CASE("StereoBalancer: +/-0.5 attenuates the opposite channel symmetrically", "[utility][balance][gate]")
{
    const auto [glNeg, grNeg] = settledBalanceGains(-0.5f);
    const auto [glPos, grPos] = settledBalanceGains(0.5f);
    REQUIRE(glNeg == Approx(1.0f).margin(1e-3));
    REQUIRE(grNeg == Approx(0.5f).margin(1e-3));
    REQUIRE(glPos == Approx(0.5f).margin(1e-3));
    REQUIRE(grPos == Approx(1.0f).margin(1e-3));
    REQUIRE(grNeg == Approx(glPos).margin(1e-3));
}

TEST_CASE("StereoWidth: width=1 is identity", "[utility][width][gate]")
{
    StereoWidth w;
    w.prepare(kSampleRate, 64);
    w.setWidth(1.0f);

    std::vector<float> L {0.6f, -0.2f, 0.9f}, R {0.1f, 0.5f, -0.4f};
    const auto L0 = L, R0 = R;
    run(w, L, R);

    for (size_t i = 0; i < L.size(); ++i)
    {
        REQUIRE(L[i] == Approx(L0[i]));
        REQUIRE(R[i] == Approx(R0[i]));
    }
}

TEST_CASE("StereoWidth: width=0 collapses to mid (mono)", "[utility][width][gate]")
{
    StereoWidth w;
    w.prepare(kSampleRate, 64);
    w.setWidth(0.0f);

    std::vector<float> L {0.6f, -0.2f, 0.9f}, R {0.1f, 0.5f, -0.4f};
    const auto L0 = L, R0 = R;
    run(w, L, R);

    for (size_t i = 0; i < L.size(); ++i)
    {
        const float mid = (L0[i] + R0[i]) * 0.5f;
        REQUIRE(L[i] == Approx(mid));
        REQUIRE(R[i] == Approx(mid));
    }
}

TEST_CASE("StereoWidth: width=2 doubles the side component", "[utility][width][gate]")
{
    StereoWidth w;
    w.prepare(kSampleRate, 64);
    w.setWidth(2.0f);

    std::vector<float> L {0.6f, -0.2f, 0.9f}, R {0.1f, 0.5f, -0.4f};
    const auto L0 = L, R0 = R;
    run(w, L, R);

    for (size_t i = 0; i < L.size(); ++i)
    {
        const float mid = (L0[i] + R0[i]) * 0.5f;
        const float side = (L0[i] - R0[i]) * 0.5f;
        REQUIRE(L[i] == Approx(mid + 2.0f * side));
        REQUIRE(R[i] == Approx(mid - 2.0f * side));
    }
}

TEST_CASE("ChannelSwap: enabled exchanges channels", "[utility][swap][gate]")
{
    ChannelSwap s;
    s.prepare(kSampleRate, 64);
    s.setEnabled(true);

    std::vector<float> L {0.5f, -0.3f, 0.7f}, R {0.2f, 0.4f, -0.6f};
    const auto L0 = L, R0 = R;
    run(s, L, R);

    REQUIRE(L == R0);
    REQUIRE(R == L0);
}

TEST_CASE("ChannelSwap: disabled is passthrough", "[utility][swap][gate]")
{
    ChannelSwap s;
    s.prepare(kSampleRate, 64);

    std::vector<float> L {0.5f, -0.3f}, R {0.2f, 0.4f};
    const auto L0 = L, R0 = R;
    run(s, L, R);

    REQUIRE(L == L0);
    REQUIRE(R == R0);
}

TEST_CASE("ChannelSolo: solo-left mutes right, left unchanged", "[utility][solo][gate]")
{
    ChannelSolo s;
    s.prepare(kSampleRate, 64);
    s.setSoloLeft(true);
    s.setSoloRight(false);

    std::vector<float> L {0.5f, -0.3f, 0.7f}, R {0.2f, 0.4f, -0.6f};
    const auto L0 = L;
    run(s, L, R);

    for (size_t i = 0; i < L.size(); ++i)
    {
        REQUIRE(L[i] == Approx(L0[i]));
        REQUIRE(R[i] == Approx(0.0f).margin(1e-7));
    }
}

TEST_CASE("ChannelSolo: solo-right mutes left, right unchanged", "[utility][solo][gate]")
{
    ChannelSolo s;
    s.prepare(kSampleRate, 64);
    s.setSoloLeft(false);
    s.setSoloRight(true);

    std::vector<float> L {0.5f, -0.3f, 0.7f}, R {0.2f, 0.4f, -0.6f};
    const auto R0 = R;
    run(s, L, R);

    for (size_t i = 0; i < L.size(); ++i)
    {
        REQUIRE(L[i] == Approx(0.0f).margin(1e-7));
        REQUIRE(R[i] == Approx(R0[i]));
    }
}

TEST_CASE("ChannelSolo: both-solo is passthrough", "[utility][solo][gate]")
{
    ChannelSolo s;
    s.prepare(kSampleRate, 64);
    s.setSoloLeft(true);
    s.setSoloRight(true);

    std::vector<float> L {0.5f, -0.3f}, R {0.2f, 0.4f};
    const auto L0 = L, R0 = R;
    run(s, L, R);

    REQUIRE(L == L0);
    REQUIRE(R == R0);
}

TEST_CASE("ChannelSolo: neither-solo is passthrough", "[utility][solo][gate]")
{
    ChannelSolo s;
    s.prepare(kSampleRate, 64);

    std::vector<float> L {0.5f, -0.3f}, R {0.2f, 0.4f};
    const auto L0 = L, R0 = R;
    run(s, L, R);

    REQUIRE(L == L0);
    REQUIRE(R == R0);
}
