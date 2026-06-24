// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#include <catch2/catch_test_macros.hpp>
#include <bw_audio_types/TransportState.h>
#include <type_traits>

using bws::domain::TransportState;

TEST_CASE("TransportState - sane defaults", "[domain][transport]")
{
    TransportState ts;
    REQUIRE(ts.sampleRate == 44100.0);
    REQUIRE(ts.samplesPerBlock == 512);
    REQUIRE(ts.bpm == 120.0);
    REQUIRE_FALSE(ts.isPlaying);
    REQUIRE(ts.samplePosition == 0);
}

TEST_CASE("TransportState - is trivially copyable", "[domain][transport]")
{
    static_assert(std::is_trivially_copyable_v<TransportState>,
                  "TransportState must be trivially copyable for RT-safety");
    REQUIRE(std::is_trivially_copyable_v<TransportState>);
}

TEST_CASE("TransportState - copy preserves values", "[domain][transport]")
{
    TransportState ts;
    ts.sampleRate = 48000.0;
    ts.samplesPerBlock = 256;
    ts.bpm = 140.0;
    ts.isPlaying = true;
    ts.samplePosition = 88200;

    TransportState copy = ts;

    REQUIRE(copy.sampleRate == 48000.0);
    REQUIRE(copy.samplesPerBlock == 256);
    REQUIRE(copy.bpm == 140.0);
    REQUIRE(copy.isPlaying);
    REQUIRE(copy.samplePosition == 88200);
}

TEST_CASE("TransportState - is standard layout", "[domain][transport]")
{
    static_assert(std::is_standard_layout_v<TransportState>, "TransportState should be standard layout for interop");
    REQUIRE(std::is_standard_layout_v<TransportState>);
}
