// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#include <catch2/catch_test_macros.hpp>
#include <bw_audio_types/ConstBufferView.h>
#include <bw_audio_types/BufferView.h>
#include <type_traits>

using bws::domain::BufferView;
using bws::domain::ConstBufferView;

TEST_CASE("ConstBufferView - default constructed is invalid", "[domain][const_buffer]")
{
    ConstBufferView view;
    REQUIRE_FALSE(view.isValid());
}

TEST_CASE("ConstBufferView - direct construction", "[domain][const_buffer]")
{
    const float left[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    const float right[4] = {5.0f, 6.0f, 7.0f, 8.0f};
    const float* channels[2] = {left, right};

    ConstBufferView view {channels, 2, 4};

    REQUIRE(view.isValid());
    REQUIRE(view.numChannels == 2);
    REQUIRE(view.numSamples == 4);
    REQUIRE(view.channel(0) == left);
    REQUIRE(view.channel(1) == right);
    REQUIRE(view.channel(0)[0] == 1.0f);
    REQUIRE(view.channel(1)[3] == 8.0f);
}

TEST_CASE("ConstBufferView - null channels is invalid", "[domain][const_buffer]")
{
    ConstBufferView view {nullptr, 2, 512};
    REQUIRE_FALSE(view.isValid());
}

TEST_CASE("ConstBufferView - zero channels is invalid", "[domain][const_buffer]")
{
    const float* ch = nullptr;
    ConstBufferView view {&ch, 0, 512};
    REQUIRE_FALSE(view.isValid());
}

TEST_CASE("ConstBufferView - implicit conversion from BufferView", "[domain][const_buffer]")
{
    float samples[64] = {};
    float* ch = samples;
    BufferView bv {&ch, 1, 64};

    ConstBufferView cbv = bv;

    REQUIRE(cbv.isValid());
    REQUIRE(cbv.numChannels == bv.numChannels);
    REQUIRE(cbv.numSamples == bv.numSamples);
    REQUIRE(cbv.channel(0) == bv.channel(0));
}

TEST_CASE("ConstBufferView - invalid BufferView converts to invalid ConstBufferView", "[domain][const_buffer]")
{
    BufferView bv;
    ConstBufferView cbv = bv;
    REQUIRE_FALSE(cbv.isValid());
}

TEST_CASE("ConstBufferView - read-only access returns correct data", "[domain][const_buffer]")
{
    const float mono[3] = {-1.0f, 0.0f, 1.0f};
    const float* channels[1] = {mono};
    ConstBufferView view {channels, 1, 3};

    REQUIRE(view.channel(0)[0] == -1.0f);
    REQUIRE(view.channel(0)[1] == 0.0f);
    REQUIRE(view.channel(0)[2] == 1.0f);
}

TEST_CASE("ConstBufferView - type safety static assertions", "[domain][const_buffer]")
{
    static_assert(std::is_constructible_v<ConstBufferView, const BufferView&>,
                  "ConstBufferView must be constructible from BufferView");
    static_assert(!std::is_constructible_v<BufferView, const ConstBufferView&>,
                  "BufferView must NOT be constructible from ConstBufferView");
    static_assert(!std::is_same_v<BufferView, ConstBufferView>,
                  "BufferView and ConstBufferView must be distinct types");

    REQUIRE(true);
}
