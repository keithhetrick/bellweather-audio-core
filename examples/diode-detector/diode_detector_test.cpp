// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

// Contract: diode-detector-modeling.md DDM-2.
// Zero has an exact floor; invalid inputs/config preserve state; reset and
// rejection can coincide. Removing validation/reset/state updates must fail.
#include "DiodeDetector.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <limits>

TEST_CASE("Diode model containment and reset", "[diode-model]")
{
    bws::experiment::DiodeDetector model;
    const auto zero = model.process(0.0);
    const auto charged = model.process(2.0);
    const double invalid =
        GENERATE(5.0, -5.0, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN());
    const auto rejected = model.process(invalid);
    const auto invalidPrepare = model.prepare(0.0);
    const auto afterInvalid = model.process(invalid);
    model.reset();
    const auto afterReset = model.process(invalid);
    const auto restarted = model.process(2.0);
    bws::experiment::DiodeDetector other;
    const auto fresh = other.process(2.0);

    // DDM-2 assertions, specified before setup/implementation.
    REQUIRE(zero.accepted);
    REQUIRE(zero.voltage == 0.0);
    REQUIRE(charged.accepted);
    REQUIRE(charged.voltage > 0.0);
    REQUIRE(charged.iterations <= 48);
    REQUIRE_FALSE(rejected.accepted);
    REQUIRE(rejected.iterations == 0);
    REQUIRE(rejected.voltage == charged.voltage);
    REQUIRE_FALSE(invalidPrepare);
    REQUIRE(afterInvalid.voltage == charged.voltage);
    REQUIRE_FALSE(afterReset.accepted);
    REQUIRE(afterReset.voltage == 0.0);
    REQUIRE(restarted.voltage == fresh.voltage);
}

// DDM-2: boundary admission and rejected-prepare/continuation overlap.
TEST_CASE("Diode preparation preserves or resets declared state", "[diode-model]")
{
    for (const double rate : {8000.0, 768000.0})
        for (const double capacitance : {1e-9, 1e-3})
        {
            bws::experiment::DiodeDetector model;
            REQUIRE(model.prepare(rate, capacitance));
            REQUIRE(model.process(2.0).accepted);
            REQUIRE(model.prepare(rate, capacitance));
            REQUIRE(model.process(0.0).voltage == 0.0);
        }
    const double invalid =
        GENERATE(-1.0, 0.0, 1e9, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN());
    bws::experiment::DiodeDetector model, twin;
    REQUIRE(model.prepare(48000.0));
    REQUIRE(twin.prepare(48000.0));
    REQUIRE(model.process(2.0).voltage == twin.process(2.0).voltage);
    REQUIRE_FALSE(model.prepare(invalid));
    REQUIRE_FALSE(model.prepare(48000.0, invalid));
    REQUIRE(model.process(1.0).voltage == twin.process(1.0).voltage);
    REQUIRE_FALSE(model.prepare(7999.0));
    REQUIRE_FALSE(model.prepare(768001.0));
    REQUIRE_FALSE(model.prepare(48000.0, 0.9e-9));
    REQUIRE_FALSE(model.prepare(48000.0, 1.1e-3));
    REQUIRE(model.process(0.0).voltage == twin.process(0.0).voltage);
}
