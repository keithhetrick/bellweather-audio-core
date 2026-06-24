// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include <bw_rt/RtSafety.h>

#include <catch2/catch_test_macros.hpp>

namespace
{

// Compile-time validation: the macros parse, expand correctly, and integrate
// with C++20 lambda/attribute placement rules. The runtime assertion is
// nominal - the value is the BUILD succeeding under -Wfunction-effects when
// the dev-fea preset is active.

void unannotated_helper() noexcept {}

void annotated_helper() noexcept BWS_NONBLOCKING {}

void allocating_helper() noexcept BWS_NONALLOCATING {}

} // namespace

TEST_CASE("RtSafety macros compile and parse", "[rt][fea][smoke]")
{
    annotated_helper();
    allocating_helper();
    unannotated_helper();

    auto nonblocking_lambda = []() noexcept BWS_NONBLOCKING {
        BWS_NONBLOCKING_UNSAFE(volatile int sink = 0; (void)sink;)
    };

    nonblocking_lambda();

    REQUIRE(true);
}
