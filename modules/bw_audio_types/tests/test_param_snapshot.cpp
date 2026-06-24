// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#include <catch2/catch_test_macros.hpp>
#include <bw_audio_types/ParamSnapshot.h>

using bws::domain::kMaxParams;
using bws::domain::ParamSnapshot;

TEST_CASE("ParamSnapshot - default initialized to zero", "[domain][params]")
{
    ParamSnapshot snap;
    REQUIRE(snap.count == 0);
    for (size_t i = 0; i < kMaxParams; ++i)
        REQUIRE(snap.values[i] == 0.0f);
}

TEST_CASE("ParamSnapshot - get returns 0 for unset params", "[domain][params]")
{
    ParamSnapshot snap;
    REQUIRE(snap.get(0) == 0.0f);
    REQUIRE(snap.get(50) == 0.0f);
    REQUIRE(snap.get(127) == 0.0f);
}

TEST_CASE("ParamSnapshot - set/get round-trip", "[domain][params]")
{
    ParamSnapshot snap;
    snap.set(0, 0.5f);
    snap.set(10, 0.75f);
    snap.set(127, 1.0f);

    REQUIRE(snap.get(0) == 0.5f);
    REQUIRE(snap.get(10) == 0.75f);
    REQUIRE(snap.get(127) == 1.0f);
}

TEST_CASE("ParamSnapshot - count advances correctly", "[domain][params]")
{
    ParamSnapshot snap;
    REQUIRE(snap.count == 0);

    snap.set(5, 0.3f);
    REQUIRE(snap.count == 6);

    snap.set(3, 0.2f);
    REQUIRE(snap.count == 6); // count doesn't decrease

    snap.set(99, 0.1f);
    REQUIRE(snap.count == 100);
}

TEST_CASE("ParamSnapshot - boundary at kMaxParams - 1", "[domain][params]")
{
    ParamSnapshot snap;
    snap.set(static_cast<uint32_t>(kMaxParams - 1), 0.42f);

    REQUIRE(snap.get(static_cast<uint32_t>(kMaxParams - 1)) == 0.42f);
    REQUIRE(snap.count == kMaxParams);
}

TEST_CASE("ParamSnapshot - out of bounds get returns 0", "[domain][params]")
{
    ParamSnapshot snap;
    snap.set(10, 0.5f);

    // id >= count returns 0
    REQUIRE(snap.get(11) == 0.0f);
    REQUIRE(snap.get(127) == 0.0f);
}

TEST_CASE("ParamSnapshot - copy is identical", "[domain][params]")
{
    ParamSnapshot original;
    original.set(0, 0.1f);
    original.set(42, 0.9f);

    ParamSnapshot copy = original;

    REQUIRE(copy.count == original.count);
    REQUIRE(copy.get(0) == original.get(0));
    REQUIRE(copy.get(42) == original.get(42));
}
