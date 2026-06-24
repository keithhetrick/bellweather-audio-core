// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#include <catch2/catch_test_macros.hpp>
#include <bw_core/BwResult.h>

using bws::domain::BwFailure;
using bws::domain::BwFailureVoid;
using bws::domain::BwResult;
using bws::domain::BwSuccess;
using bws::domain::BwSuccessVoid;

namespace
{
struct TestPayload
{
    int code = 0;
    float level = 0.0f;
};
} // namespace

TEST_CASE("BwResult<T> - success construction", "[domain][result]")
{
    auto r = BwSuccess(TestPayload {42, 3.14f});
    REQUIRE(r.has_value());
    REQUIRE(r->code == 42);
    REQUIRE(r->level == 3.14f);
}

TEST_CASE("BwResult<T> - failure construction", "[domain][result]")
{
    auto r = BwFailure<TestPayload>("something broke");
    REQUIRE_FALSE(r.has_value());
    REQUIRE(r.error() == "something broke");
}

TEST_CASE("BwResult<int> - success with primitive", "[domain][result]")
{
    auto r = BwSuccess(99);
    REQUIRE(r.has_value());
    REQUIRE(*r == 99);
}

TEST_CASE("BwResult<int> - failure with primitive", "[domain][result]")
{
    auto r = BwFailure<int>("invalid");
    REQUIRE_FALSE(r.has_value());
    REQUIRE(r.error() == "invalid");
}

TEST_CASE("BwResult<void> - success", "[domain][result]")
{
    auto r = BwSuccessVoid();
    REQUIRE(r.has_value());
}

TEST_CASE("BwResult<void> - failure", "[domain][result]")
{
    auto r = BwFailureVoid("disk full");
    REQUIRE_FALSE(r.has_value());
    REQUIRE(r.error() == "disk full");
}

TEST_CASE("BwResult<T> - operator bool", "[domain][result]")
{
    auto ok = BwSuccess(42);
    auto err = BwFailure<int>("bad");
    REQUIRE(static_cast<bool>(ok));
    REQUIRE_FALSE(static_cast<bool>(err));
}

// Error-access is the one documented divergence between the two paths.
#if BW_HAS_EXPECTED
TEST_CASE("BwResult<T> - error-access throws on the std::expected path", "[domain][result][fallback-divergence]")
{
    auto err = BwFailure<int>("bad");
    REQUIRE_THROWS(static_cast<void>(err.value()));
}
#else
TEST_CASE("BwResult<T> - error-access returns default on the C++20 fallback path",
          "[domain][result][fallback-divergence]")
{
    auto err = BwFailure<int>("bad");
    CHECK(err.value() == int {});
}
#endif
