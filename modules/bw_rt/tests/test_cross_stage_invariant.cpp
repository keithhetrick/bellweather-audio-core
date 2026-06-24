// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
//
// CrossStageInvariant.h unit + property tests (R3 cross-stage invariant
// pilot; bw_rt-side coverage). See:
//
//
// Stage-specific pilot invariants (LA + crossfade seam) live in
// plugins/production//tests/invariant/ and consume the
// primitives validated here.
//
// Bias-resistance: each test case derives its expected behavior from
// the invariant's documented derivation (Parseval's theorem; JUCE
// latency contract; causal scheduler property) - NOT from observed
// behavior of any production stage.

#include <bw_rt/CrossStageInvariant.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>

using bws::rt::CrossStageInvariantSet;
using bws::rt::CrossStageSnapshot;
using bws::rt::makeEnergyConservationInvariant;
using bws::rt::makeLatencyConsistencyInvariant;
using bws::rt::makeScheduleMonotonicityInvariant;

namespace
{
CrossStageSnapshot makeSnap(std::string name, double reported_latency, double consumed_latency, std::int64_t ts,
                            double energy)
{
    CrossStageSnapshot s;
    s.stage_name = std::move(name);
    s.latency_reported_samples = reported_latency;
    s.latency_consumed_samples = consumed_latency;
    s.schedule_timestamp_samples = ts;
    s.energy_in = energy;
    s.energy_out = energy;
    s.sample_rate_hz = 48000.0;
    s.channel_count = 2;
    return s;
}
} // namespace

TEST_CASE("latency consistency invariant: equal reported/consumed passes", "[cross_stage_invariant][latency]")
{
    auto inv = makeLatencyConsistencyInvariant();
    auto before = makeSnap("up", 240.0, 0.0, 0, 1.0);
    auto after = makeSnap("down", 0.0, 240.0, 0, 1.0);
    auto r = inv(before, after);
    REQUIRE(r.passed);
}

TEST_CASE("latency consistency invariant: mismatch beyond tolerance fails", "[cross_stage_invariant][latency]")
{
    auto inv = makeLatencyConsistencyInvariant(0.5);
    auto before = makeSnap("up", 240.0, 0.0, 0, 1.0);
    auto after = makeSnap("down", 0.0, 239.0, 0, 1.0); // off by 1.0 > tol 0.5
    auto r = inv(before, after);
    REQUIRE_FALSE(r.passed);
    REQUIRE(r.diagnostic.find("latency mismatch") != std::string::npos);
}

TEST_CASE("latency consistency invariant: sub-sample drift within tolerance passes", "[cross_stage_invariant][latency]")
{
    auto inv = makeLatencyConsistencyInvariant(0.5);
    auto before = makeSnap("up", 240.0, 0.0, 0, 1.0);
    auto after = makeSnap("down", 0.0, 240.25, 0, 1.0); // sub-sample fractional
    auto r = inv(before, after);
    REQUIRE(r.passed);
}

TEST_CASE("schedule monotonicity invariant: same-timestamp passes", "[cross_stage_invariant][schedule]")
{
    auto inv = makeScheduleMonotonicityInvariant();
    auto before = makeSnap("a", 0, 0, 1000, 1.0);
    auto after = makeSnap("b", 0, 0, 1000, 1.0);
    REQUIRE(inv(before, after).passed);
}

TEST_CASE("schedule monotonicity invariant: forward progress passes", "[cross_stage_invariant][schedule]")
{
    auto inv = makeScheduleMonotonicityInvariant();
    auto before = makeSnap("a", 0, 0, 1000, 1.0);
    auto after = makeSnap("b", 0, 0, 1064, 1.0);
    REQUIRE(inv(before, after).passed);
}

TEST_CASE("schedule monotonicity invariant: regression fails", "[cross_stage_invariant][schedule]")
{
    auto inv = makeScheduleMonotonicityInvariant();
    auto before = makeSnap("a", 0, 0, 1000, 1.0);
    auto after = makeSnap("b", 0, 0, 999, 1.0);
    auto r = inv(before, after);
    REQUIRE_FALSE(r.passed);
    REQUIRE(r.diagnostic.find("schedule regression") != std::string::npos);
}

TEST_CASE("energy conservation invariant: passive seam preserves energy", "[cross_stage_invariant][energy]")
{
    auto inv = makeEnergyConservationInvariant();
    auto before = makeSnap("a", 0, 0, 0, 1.0);
    auto after = makeSnap("b", 0, 0, 0, 1.0);
    REQUIRE(inv(before, after).passed);
}

TEST_CASE("energy conservation invariant: gain factor handles known attenuation", "[cross_stage_invariant][energy]")
{
    auto inv = makeEnergyConservationInvariant(1e-6, 0.5);
    auto before = makeSnap("a", 0, 0, 0, 1.0);
    auto after = makeSnap("b", 0, 0, 0, 0.5); // matches gain_factor
    REQUIRE(inv(before, after).passed);
}

TEST_CASE("energy conservation invariant: drift beyond epsilon fails", "[cross_stage_invariant][energy]")
{
    auto inv = makeEnergyConservationInvariant(1e-6);
    auto before = makeSnap("a", 0, 0, 0, 1.0);
    auto after = makeSnap("b", 0, 0, 0, 0.5); // 50% drop with no gain factor
    auto r = inv(before, after);
    REQUIRE_FALSE(r.passed);
    REQUIRE(r.diagnostic.find("energy drift") != std::string::npos);
}

TEST_CASE("aggregate evaluation: all-pass composite returns passed", "[cross_stage_invariant][aggregate]")
{
    CrossStageInvariantSet set;
    set.registerInvariant("latency", "JUCE latency contract", makeLatencyConsistencyInvariant());
    set.registerInvariant("monotonicity", "causal scheduler property", makeScheduleMonotonicityInvariant());
    set.registerInvariant("energy", "Parseval's theorem", makeEnergyConservationInvariant());

    auto before = makeSnap("a", 240.0, 0, 1000, 1.0);
    auto after = makeSnap("b", 0, 240.0, 1064, 1.0);
    auto agg = set.evaluateAll(before, after);
    REQUIRE(agg.passed);
    REQUIRE_FALSE(agg.first_failure_name.has_value());
    REQUIRE(set.size() == 3);
}

TEST_CASE("aggregate evaluation: first failure surfaces; all failures collected", "[cross_stage_invariant][aggregate]")
{
    CrossStageInvariantSet set;
    set.registerInvariant("latency", "JUCE latency contract", makeLatencyConsistencyInvariant(0.5));
    set.registerInvariant("monotonicity", "causal scheduler property", makeScheduleMonotonicityInvariant());
    set.registerInvariant("energy", "Parseval's theorem", makeEnergyConservationInvariant(1e-6));

    auto before = makeSnap("a", 240.0, 0, 1000, 1.0);
    // Three deliberate failures: latency off by 10, schedule regresses, energy drops 50%
    auto after = makeSnap("b", 0, 250.0, 999, 0.5);
    auto agg = set.evaluateAll(before, after);
    REQUIRE_FALSE(agg.passed);
    REQUIRE(agg.first_failure_name.has_value());
    REQUIRE(*agg.first_failure_name == "latency");
    REQUIRE(agg.all_failures.size() == 3);
}

TEST_CASE("property: latency invariant passes for any random equal pair within tolerance",
          "[cross_stage_invariant][property]")
{
    // Native Catch2 GENERATE in place of RapidCheck per native-first preference.
    auto reported = GENERATE(take(20, random(0.0, 4096.0)));
    auto consumed = reported; // identity = always within any positive tolerance
    auto inv = makeLatencyConsistencyInvariant(0.5);
    auto before = makeSnap("up", reported, 0, 0, 1.0);
    auto after = makeSnap("down", 0, consumed, 0, 1.0);
    REQUIRE(inv(before, after).passed);
}

TEST_CASE("property: monotonicity invariant catches every random regression", "[cross_stage_invariant][property]")
{
    auto base = GENERATE(take(20, random(1000, 1000000)));
    auto regression = GENERATE(take(5, random(1, 500)));
    auto inv = makeScheduleMonotonicityInvariant();
    auto before = makeSnap("a", 0, 0, base, 1.0);
    auto after = makeSnap("b", 0, 0, base - regression, 1.0);
    REQUIRE_FALSE(inv(before, after).passed);
}
