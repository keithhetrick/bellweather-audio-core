// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#include <catch2/catch_test_macros.hpp>
#include <bw_audio_types/BufferView.h>

using bws::domain::BufferView;

TEST_CASE("BufferView - default constructed is invalid", "[domain][buffer]")
{
    BufferView view;
    REQUIRE_FALSE(view.isValid());
}

TEST_CASE("BufferView - null channels is invalid", "[domain][buffer]")
{
    BufferView view {nullptr, 2, 512};
    REQUIRE_FALSE(view.isValid());
}

TEST_CASE("BufferView - zero channels is invalid", "[domain][buffer]")
{
    float* ch = nullptr;
    BufferView view {&ch, 0, 512};
    REQUIRE_FALSE(view.isValid());
}

TEST_CASE("BufferView - zero samples is invalid", "[domain][buffer]")
{
    float* ch = nullptr;
    BufferView view {&ch, 1, 0};
    REQUIRE_FALSE(view.isValid());
}

TEST_CASE("BufferView - valid stereo buffer", "[domain][buffer]")
{
    float left[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    float right[4] = {5.0f, 6.0f, 7.0f, 8.0f};
    float* channels[2] = {left, right};

    BufferView view {channels, 2, 4};

    REQUIRE(view.isValid());
    REQUIRE(view.numChannels == 2);
    REQUIRE(view.numSamples == 4);

    REQUIRE(view.channel(0) == left);
    REQUIRE(view.channel(1) == right);
    REQUIRE(view.channel(0)[0] == 1.0f);
    REQUIRE(view.channel(1)[3] == 8.0f);
}

TEST_CASE("BufferView - mono buffer", "[domain][buffer]")
{
    float mono[2] = {-1.0f, 1.0f};
    float* channels[1] = {mono};

    BufferView view {channels, 1, 2};

    REQUIRE(view.isValid());
    REQUIRE(view.channel(0)[0] == -1.0f);
    REQUIRE(view.channel(0)[1] == 1.0f);
}
