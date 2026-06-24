// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
// Contract source: bw_audio_types/ProcessContext.h
// Gate C: ProcessContext seam type contract tests

#include <bw_audio_types/ProcessContext.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using bws::domain::BufferView;
using bws::domain::ParamSnapshot;
using bws::domain::ProcessContext;
using bws::domain::TransportState;
using Catch::Matchers::WithinAbs;

// =========================================================================
// Positive tests
// =========================================================================

TEST_CASE("ProcessContext - default constructed has valid defaults", "[domain][seam]")
{
    ProcessContext ctx;
    REQUIRE(ctx.transport.sampleRate == 44100.0);
    REQUIRE(ctx.transport.bpm == 120.0);
    REQUIRE(ctx.transport.isPlaying == false);
    REQUIRE(ctx.params.count == 0);
    REQUIRE_FALSE(ctx.buffer.isValid());
}

TEST_CASE("ProcessContext - param snapshot stores plain values", "[domain][seam]")
{
    ProcessContext ctx;
    // Plain values - dB, ms, Hz, ratios - not normalized [0,1]
    ctx.params.set(0, -20.0f); // threshold: -20 dB
    ctx.params.set(1, 4.0f);   // ratio: 4:1
    ctx.params.set(2, 100.0f); // mix: 100%
    ctx.params.set(3, 30.0f);  // filter: 30 Hz
    REQUIRE_THAT(ctx.params.get(0), WithinAbs(-20.0f, 1e-6f));
    REQUIRE_THAT(ctx.params.get(1), WithinAbs(4.0f, 1e-6f));
    REQUIRE_THAT(ctx.params.get(2), WithinAbs(100.0f, 1e-6f));
    REQUIRE_THAT(ctx.params.get(3), WithinAbs(30.0f, 1e-6f));
    REQUIRE(ctx.params.count > 0);
}

TEST_CASE("ProcessContext - transport carries BPM", "[domain][seam]")
{
    ProcessContext ctx;
    ctx.transport.bpm = 140.0;
    ctx.transport.isPlaying = true;
    ctx.transport.samplePosition = 48000;
    REQUIRE(ctx.transport.bpm == 140.0);
    REQUIRE(ctx.transport.isPlaying == true);
    REQUIRE(ctx.transport.samplePosition == 48000);
}

TEST_CASE("ProcessContext - buffer view wired correctly", "[domain][seam]")
{
    float left[4] = {0.1f, 0.2f, 0.3f, 0.4f};
    float right[4] = {0.5f, 0.6f, 0.7f, 0.8f};
    float* channels[2] = {left, right};

    ProcessContext ctx;
    ctx.buffer = {channels, 2, 4};
    REQUIRE(ctx.buffer.isValid());
    REQUIRE(ctx.buffer.numChannels == 2);
    REQUIRE(ctx.buffer.numSamples == 4);
    REQUIRE_THAT(ctx.buffer.channel(0)[2], WithinAbs(0.3f, 1e-6f));
    REQUIRE_THAT(ctx.buffer.channel(1)[3], WithinAbs(0.8f, 1e-6f));
}

// =========================================================================
// Negative tests (required per testing contracts)
// =========================================================================

TEST_CASE("ProcessContext - out-of-range param returns 0", "[domain][seam]")
{
    ProcessContext ctx;
    ctx.params.set(0, 0.5f);
    REQUIRE(ctx.params.get(99) == 0.0f);
    REQUIRE(ctx.params.get(127) == 0.0f);
}

TEST_CASE("ProcessContext - null buffer is invalid", "[domain][seam]")
{
    ProcessContext ctx;
    ctx.buffer = {nullptr, 2, 512};
    REQUIRE_FALSE(ctx.buffer.isValid());
}

TEST_CASE("ProcessContext - zero-sample buffer is invalid", "[domain][seam]")
{
    float ch = 0.0f;
    float* channels[1] = {&ch};
    ProcessContext ctx;
    ctx.buffer = {channels, 1, 0};
    REQUIRE_FALSE(ctx.buffer.isValid());
}
