// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#include <catch2/catch_test_macros.hpp>
#include <bw_state_types/BwStateBlob.h>
#include <array>

using bws::domain::BwStateBlob;

TEST_CASE("BwStateBlob - default constructed is empty", "[domain][state]")
{
    BwStateBlob blob;
    REQUIRE(blob.isEmpty());
    REQUIRE(blob.size() == 0);
}

TEST_CASE("BwStateBlob - version defaults to 1", "[domain][state]")
{
    BwStateBlob blob;
    REQUIRE(blob.version == 1);
}

TEST_CASE("BwStateBlob - size matches span", "[domain][state]")
{
    std::array<std::byte, 16> storage {};
    BwStateBlob blob {std::span<const std::byte>(storage), 1};

    REQUIRE_FALSE(blob.isEmpty());
    REQUIRE(blob.size() == 16);
}

TEST_CASE("BwStateBlob - custom version", "[domain][state]")
{
    std::array<std::byte, 4> storage {};
    BwStateBlob blob {std::span<const std::byte>(storage), 2};

    REQUIRE(blob.version == 2);
    REQUIRE(blob.size() == 4);
}

TEST_CASE("BwStateBlob - empty span is empty", "[domain][state]")
{
    BwStateBlob blob {std::span<const std::byte> {}, 1};
    REQUIRE(blob.isEmpty());
    REQUIRE(blob.size() == 0);
}
