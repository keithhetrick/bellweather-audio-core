// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
// Public contract tests for LockFreeRingBuffer.

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "bw_rt/LockFreeRingBuffer.h"
#include <vector>

using namespace bws::rt;

TEST_CASE("LockFreeRingBuffer prepare and initial state", "[ring_buffer]")
{
    LockFreeRingBuffer buf;
    buf.prepare(2, 512);

    SECTION("available to write equals capacity after prepare")
    {
        REQUIRE(buf.availableToWriteSamples() == 512);
    }

    SECTION("available to read is zero after prepare")
    {
        REQUIRE(buf.availableToReadSamples() == 0);
    }

    SECTION("write counter starts at zero")
    {
        REQUIRE(buf.getWriteCounter() == 0);
    }

    SECTION("capacity and channels are set")
    {
        REQUIRE(buf.getCapacity() == 512);
        REQUIRE(buf.getNumChannels() == 2);
    }
}

TEST_CASE("LockFreeRingBuffer push reduces write space", "[ring_buffer]")
{
    LockFreeRingBuffer buf;
    buf.prepare(1, 64);

    std::vector<float> samples(32, 0.5f);
    const float* ch = samples.data();

    SECTION("push advances write counter")
    {
        buf.push(&ch, 1, 32);
        REQUIRE(buf.getWriteCounter() == 32);
    }

    SECTION("available to write decreases after push")
    {
        buf.push(&ch, 1, 32);
        REQUIRE(buf.availableToWriteSamples() == 32);
    }

    SECTION("available to read increases after push")
    {
        buf.push(&ch, 1, 32);
        REQUIRE(buf.availableToReadSamples() == 32);
    }

    SECTION("push returns true when space available")
    {
        REQUIRE(buf.push(&ch, 1, 32));
    }

    SECTION("push returns false when buffer full")
    {
        std::vector<float> full(64, 1.0f);
        const float* fullCh = full.data();
        buf.push(&fullCh, 1, 64);
        REQUIRE_FALSE(buf.push(&ch, 1, 1));
    }
}

TEST_CASE("LockFreeRingBuffer push then readChannel round-trip", "[ring_buffer]")
{
    LockFreeRingBuffer buf;
    buf.prepare(1, 64);

    std::vector<float> input = {1.0f, 2.0f, 3.0f, 4.0f};
    const float* ch = input.data();
    buf.push(&ch, 1, static_cast<int>(input.size()));

    SECTION("readChannel retrieves pushed samples")
    {
        std::vector<float> output(4, 0.0f);
        buf.readChannel(0, output.data(), 4);
        REQUIRE(output[0] == Catch::Approx(1.0f));
        REQUIRE(output[1] == Catch::Approx(2.0f));
        REQUIRE(output[2] == Catch::Approx(3.0f));
        REQUIRE(output[3] == Catch::Approx(4.0f));
    }

    SECTION("consumeSamples restores write space")
    {
        buf.consumeSamples(static_cast<int>(input.size()));
        REQUIRE(buf.availableToWriteSamples() == 64);
        REQUIRE(buf.availableToReadSamples() == 0);
    }
}

TEST_CASE("LockFreeRingBuffer channel clamping", "[ring_buffer]")
{
    SECTION("numChannels clamped to minimum 1")
    {
        LockFreeRingBuffer buf;
        buf.prepare(0, 64);
        REQUIRE(buf.getNumChannels() == 1);
        REQUIRE(buf.availableToWriteSamples() == 64);
    }

    SECTION("numChannels clamped to maximum 2")
    {
        LockFreeRingBuffer buf;
        buf.prepare(8, 64);
        REQUIRE(buf.getNumChannels() == 2);
        REQUIRE(buf.availableToWriteSamples() == 64);
    }
}

TEST_CASE("LockFreeRingBuffer readChannel out of bounds", "[ring_buffer]")
{
    LockFreeRingBuffer buf;
    buf.prepare(1, 32);

    std::vector<float> input(16, 0.5f);
    const float* ch = input.data();
    buf.push(&ch, 1, 16);

    SECTION("reading invalid channel does not crash")
    {
        std::vector<float> output(16, -1.0f);
        buf.readChannel(-1, output.data(), 16);
        REQUIRE(output[0] == Catch::Approx(-1.0f));
    }

    SECTION("reading channel beyond numChannels does not crash")
    {
        std::vector<float> output(16, -1.0f);
        buf.readChannel(5, output.data(), 16);
        REQUIRE(output[0] == Catch::Approx(-1.0f));
    }
}

TEST_CASE("LockFreeRingBuffer wraps at non-power-of-two capacity without data loss", "[ring_buffer]")
{
    LockFreeRingBuffer buf;
    buf.prepare(1, 5);

    std::vector<float> out(3, 0.0f);
    for (int round = 0; round < 7; ++round)
    {
        const auto v = static_cast<float>(round * 3);
        std::vector<float> in = {v, v + 1.0f, v + 2.0f};
        const float* ch = in.data();
        REQUIRE(buf.push(&ch, 1, 3));

        buf.readChannel(0, out.data(), 3);
        CHECK(out[0] == Catch::Approx(v));
        CHECK(out[1] == Catch::Approx(v + 1.0f));
        CHECK(out[2] == Catch::Approx(v + 2.0f));

        buf.consumeSamples(3);
        CHECK(buf.availableToReadSamples() == 0);
        CHECK(buf.availableToWriteSamples() == buf.getCapacity());
    }
}

TEST_CASE("LockFreeRingBuffer wraps across capacity without data loss", "[ring_buffer]")
{
    LockFreeRingBuffer buf;
    buf.prepare(1, 4);

    std::vector<float> out(2, 0.0f);
    for (int round = 0; round < 6; ++round)
    {
        const auto v = static_cast<float>(round + 1);
        std::vector<float> in = {v, v + 0.5f};
        const float* ch = in.data();
        REQUIRE(buf.push(&ch, 1, 2));

        buf.readChannel(0, out.data(), 2);
        CHECK(out[0] == Catch::Approx(v));
        CHECK(out[1] == Catch::Approx(v + 0.5f));

        buf.consumeSamples(2);
        CHECK(buf.availableToReadSamples() <= buf.getCapacity());
        CHECK(buf.availableToWriteSamples() <= buf.getCapacity());
    }
}
