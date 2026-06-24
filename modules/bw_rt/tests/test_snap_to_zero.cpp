// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
// Public contract tests for snapToZero.

// Contract: modules/bw_rt/include/bw_rt/SnapToZero.h - flush magnitudes below
// kSnapThreshold to exactly zero; pass everything else through unchanged.

#include "bw_rt/SnapToZero.h"

#include <catch2/catch_test_macros.hpp>

#include <array>

using bws::rt::kSnapThreshold;
using bws::rt::snapToZero;

TEST_CASE("snapToZero floors magnitudes below the threshold", "[snap_to_zero]")
{
    CHECK(snapToZero(0.0f) == 0.0f);
    CHECK(snapToZero(kSnapThreshold * 0.5f) == 0.0f);
    CHECK(snapToZero(-kSnapThreshold * 0.5f) == 0.0f);
    CHECK(snapToZero(1.0e-30f) == 0.0f);
}

TEST_CASE("snapToZero passes through values at or above the threshold", "[snap_to_zero]")
{
    CHECK(snapToZero(1.0f) == 1.0f);
    CHECK(snapToZero(-0.25f) == -0.25f);
    CHECK(snapToZero(kSnapThreshold) == kSnapThreshold);
}

TEST_CASE("snapToZero batch snaps each element independently", "[snap_to_zero]")
{
    std::array<float, 4> data {1.0e-30f, 0.5f, -1.0e-25f, -2.0f};
    snapToZero(data.data(), data.size());
    CHECK(data[0] == 0.0f);
    CHECK(data[1] == 0.5f);
    CHECK(data[2] == 0.0f);
    CHECK(data[3] == -2.0f);
}
