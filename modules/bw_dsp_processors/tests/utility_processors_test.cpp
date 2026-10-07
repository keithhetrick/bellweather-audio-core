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
#include <cmath>
#include <limits>
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

// Contract: CS-1/3/4/5.
// Component/public seam; nonzero DC reveals every transition sample without
// host/codec ambiguity. Good: prescribed envelope. Bad: hard-switch old code.
// A repeated target in uneven callbacks must not restart either ramp.
// Tolerance: contract floating-point budget; exact unaffected/settled samples.
TEST_CASE("ChannelSolo: runtime mute and unmute follow sample time", "[utility][solo][solo_transition][gate]")
{
    for (const double sampleRate : {44100.0, 48000.0, 96000.0})
    {
        const int n = static_cast<int>(std::round(sampleRate * 0.010));
        const double tolerance = 2.0 * n * std::numeric_limits<float>::epsilon();
        ChannelSolo solo;
        solo.prepare(sampleRate, 1024);
        std::vector<float> left(1, 1.0f), right(1, 1.0f);
        run(solo, left, right); // Establish playing state before runtime change.
        for (const bool muted : {true, false})
        {
            const float target = muted ? 0.0f : 1.0f;
            int elapsed = 0;
            for (const int block : {1, 17, 63, 1024})
            {
                solo.setSoloLeft(muted);
                left.assign(block, 1.0f);
                right.assign(block, 1.0f);
                run(solo, left, right);
                for (size_t i = 0; i < right.size(); ++i)
                {
                    const double fraction = std::min(1.0, double(++elapsed) / n);
                    const double expected = muted ? 1.0 - fraction : fraction;
                    CAPTURE(sampleRate, muted, block, elapsed, expected, right[i]);
                    REQUIRE(left[i] == 1.0f);
                    REQUIRE(std::abs(right[i] - expected) <= tolerance);
                }
            }
            REQUIRE(right.back() == target);
        }
    }
}

// CS-3/4: two channel ramps overlap; changing one target must not restart the
// other. Closed-form quarter-ramp values are independent of production state.
// Bad witnesses: shared state, swapped targets, snap-on-reversal, restart.
TEST_CASE("ChannelSolo: reversal overlaps an independently continuing channel",
          "[utility][solo][solo_transition][gate]")
{
    ChannelSolo solo;
    solo.prepare(48000, 1024);
    solo.setSoloLeft(true);
    std::vector<float> left(1, 1), right(1, 1);
    run(solo, left, right);
    solo.setSoloLeft(false);
    solo.setSoloRight(true);
    left.assign(120, 1);
    right.assign(120, 1);
    run(solo, left, right);
    const double tolerance = 960.0 * std::numeric_limits<float>::epsilon();
    for (int i = 0; i < 120; ++i)
    {
        CAPTURE(i, left[i], right[i]);
        REQUIRE(std::abs(left[i] - (1.0 - (i + 1) / 480.0)) <= tolerance);
        REQUIRE(std::abs(right[i] - (i + 1) / 480.0) <= tolerance);
    }
    solo.setSoloLeft(true); // Both: left reverses, right continues its old ramp.
    left.assign(481, 1);
    right.assign(481, 1);
    run(solo, left, right);
    for (int i = 0; i < 481; ++i)
    {
        const double expectedLeft = 0.75 + 0.25 * std::min(1.0, (i + 1) / 480.0);
        const double expectedRight = std::min(1.0, (i + 121) / 480.0);
        CAPTURE(i, expectedLeft, expectedRight, left[i], right[i]);
        REQUIRE(std::abs(left[i] - expectedLeft) <= tolerance);
        REQUIRE(std::abs(right[i] - expectedRight) <= tolerance);
    }
    REQUIRE(left.back() == 1.0f);
    REQUIRE(right.back() == 1.0f);
}

// CS-2/4: lifecycle overlaps an unfinished ramp. Reset flags and first-call
// snapping are independently observed, then the configured duration is reused.
// Bad witnesses: paired flag-reset deletion, stale initialized/ramp state.
TEST_CASE("ChannelSolo: reset and reprepare discard unfinished transitions", "[utility][solo][solo_transition][gate]")
{
    for (const bool reprepare : {false, true})
    {
        ChannelSolo solo;
        solo.prepare(48000, 1024);
        std::vector<float> left(1, 1), right(1, 1);
        run(solo, left, right);
        solo.setSoloLeft(true);
        left.assign(120, 1);
        right.assign(120, 1);
        run(solo, left, right);
        solo.setSoloRight(true);
        if (reprepare)
            solo.prepare(96000, 1024);
        else
            solo.reset();
        REQUIRE_FALSE(solo.isLeftSoloed());
        REQUIRE_FALSE(solo.isRightSoloed());
        solo.setSoloRight(true); // Simulate host republishing restored state.
        left.assign(1, 1);
        right.assign(1, 1);
        run(solo, left, right);
        REQUIRE(left[0] == 0.0f);
        REQUIRE(right[0] == 1.0f);
        solo.setSoloRight(false);
        const int n = reprepare ? 960 : 480;
        left.assign(n, 1);
        right.assign(n, 1);
        run(solo, left, right);
        const double tolerance = 2.0 * n * std::numeric_limits<float>::epsilon();
        for (int i = 0; i < n; ++i)
        {
            CAPTURE(reprepare, n, i, left[i]);
            REQUIRE(std::abs(left[i] - (i + 1.0) / n) <= tolerance);
            REQUIRE(right[i] == 1.0f);
        }
        REQUIRE(left.back() == 1.0f);
    }
}

// CS-2/4: empty and mono calls neither initialize nor consume ramp time.
// Bad witnesses: initialization before guard, advancing on non-stereo buffers.
TEST_CASE("ChannelSolo: empty and mono calls consume no stereo time", "[utility][solo][solo_transition][gate]")
{
    ChannelSolo solo;
    solo.prepare(48000, 64);
    std::array<float, 32> mono;
    mono.fill(0.75f);
    std::array<float*, 2> channels {mono.data(), mono.data()};
    solo.process({channels.data(), 1, 32});
    solo.process({channels.data(), 2, 0});
    solo.setSoloLeft(true);
    std::vector<float> left(1, 1), right(1, 1);
    run(solo, left, right);
    REQUIRE(right[0] == 0.0f);
    solo.setSoloLeft(false);
    left.assign(120, 1);
    right.assign(120, 1);
    run(solo, left, right);
    solo.process({channels.data(), 1, 32});
    solo.process({channels.data(), 2, 0});
    for (const float sample : mono)
        REQUIRE(sample == 0.75f);
    left.assign(1, 1);
    right.assign(1, 1);
    run(solo, left, right);
    CAPTURE(right[0]);
    REQUIRE(std::abs(right[0] - 121.0 / 480.0) <= 960.0 * std::numeric_limits<float>::epsilon());
    REQUIRE(left[0] == 1.0f);
}

// CS-1/3/5: +12 dB continuous input and distinct sidechain channels remain
// legal throughout active and settled ramps. No amplitude ceiling is assumed.
// Bad witnesses: writing every channel, clipping to unity, skipping one side.
TEST_CASE("ChannelSolo: four channels and high-level input preserve isolation",
          "[utility][solo][solo_transition][gate]")
{
    for (const bool soloLeft : {false, true})
    {
        ChannelSolo solo;
        solo.prepare(48000, 1024);
        const float amplitude = std::pow(10.0f, 12.0f / 20.0f);
        std::array<std::vector<float>, 4> data;
        std::array<float*, 4> channels;
        for (int c = 0; c < 4; ++c)
        {
            data[c].assign(1024, amplitude * (c + 1));
            channels[c] = data[c].data();
        }
        solo.process({channels.data(), 4, 1});
        solo.setSoloLeft(soloLeft);
        solo.setSoloRight(!soloLeft);
        for (int block = 0; block < 2; ++block)
        {
            for (int c = 0; c < 4; ++c)
                std::fill(data[c].begin(), data[c].end(), amplitude * (c + 1));
            solo.process({channels.data(), 4, 1024});
            for (int c = 0; c < 4; ++c)
                for (int i = 0; i < 1024; ++i)
                {
                    const bool muted = c == (soloLeft ? 1 : 0);
                    const double gain = !muted ? 1.0 : block == 1 ? 0.0 : std::max(0.0, 1.0 - (i + 1) / 480.0);
                    const float source = amplitude * (c + 1);
                    const double expected = source * gain;
                    CAPTURE(soloLeft, block, c, i, expected, data[c][i]);
                    if (!muted || gain == 0.0)
                        REQUIRE(data[c][i] == static_cast<float>(expected));
                    else
                        REQUIRE(std::abs(data[c][i] - expected) <=
                                std::abs(source) * 960.0 * std::numeric_limits<float>::epsilon());
                }
        }
    }
}

// CS-3 lower boundary: valid small rate rounds to N=1; runtime transition
// reaches the exact target on its first sample. Bad: zero-step/stale ramp.
TEST_CASE("ChannelSolo: minimum ramp reaches its endpoint in one sample", "[utility][solo][solo_transition][gate]")
{
    ChannelSolo solo;
    solo.prepare(1.0, 1);
    std::vector<float> left(1, 1), right(1, 1);
    run(solo, left, right);
    solo.setSoloLeft(true);
    run(solo, left, right);
    REQUIRE(right[0] == 0.0f);
    solo.setSoloLeft(false);
    right[0] = 1;
    run(solo, left, right);
    REQUIRE(right[0] == 1.0f);
    REQUIRE(left[0] == 1.0f);
}

// CS-3/4: both independent gains must preserve progress when their target is
// republished on every callback. The left restart mutation is the bad witness.
TEST_CASE("ChannelSolo: repeated right Solo cannot restart the left ramp", "[utility][solo][solo_transition][gate]")
{
    ChannelSolo solo;
    solo.prepare(48000, 64);
    std::vector<float> left(1, 1), right(1, 1);
    run(solo, left, right);
    for (int sample = 1; sample <= 481; ++sample)
    {
        solo.setSoloRight(true);
        left[0] = right[0] = 1;
        run(solo, left, right);
        const double expected = std::max(0.0, 1.0 - sample / 480.0);
        CAPTURE(sample, expected, left[0]);
        REQUIRE(std::abs(left[0] - expected) <= 960.0 * std::numeric_limits<float>::epsilon());
        REQUIRE(right[0] == 1.0f);
    }
    REQUIRE(left[0] == 0.0f);
}
