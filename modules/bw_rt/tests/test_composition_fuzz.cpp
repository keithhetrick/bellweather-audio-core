// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
//
// CompositionFuzz.h unit tests (R4 composition fuzzer pilot; bw_rt-side
// coverage). See:
//
//
// Stage-specific pilot at 's pipeline lives in
// plugins/production//tests/composition_fuzz/ and consumes
// the harness validated here.
//
// Bias-resistance: synthetic stages used in these tests have closed-form
// behavior (pass-through, scalar-gain, additive-shift). Invariant
// derivations match the synthetic stage math, NOT what makes the test
// pass.

#include <bw_rt/CompositionFuzz.h>
#include <bw_rt/CrossStageInvariant.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>

using bws::rt::allPermutations;
using bws::rt::CompositionResult;
using bws::rt::CrossStageInvariantSet;
using bws::rt::CrossStageSnapshot;
using bws::rt::evaluateOrdering;
using bws::rt::FuzzVerdict;
using bws::rt::makeEnergyConservationInvariant;
using bws::rt::makeScheduleMonotonicityInvariant;
using bws::rt::runCompositionFuzz;
using bws::rt::SafePermutationSet;
using bws::rt::StageAdapter;
using bws::rt::StageId;
using bws::rt::StageOrdering;

namespace
{
// Build a synthetic pass-through stage with constant snapshot.
StageAdapter makePassThrough(StageId id, std::string name, std::int64_t ts = 0)
{
    StageAdapter a;
    a.id = id;
    a.name = name;
    a.process = [](double e) {
        return e;
    };
    a.snapshot = [name, ts]() {
        CrossStageSnapshot s;
        s.stage_name = name;
        s.schedule_timestamp_samples = ts;
        s.energy_in = 1.0;
        s.energy_out = 1.0;
        return s;
    };
    return a;
}
} // namespace

TEST_CASE("allPermutations: 3-element set yields 6 orderings", "[composition_fuzz][permutations]")
{
    auto perms = allPermutations({0, 1, 2});
    REQUIRE(perms.size() == 6);
}

TEST_CASE("allPermutations: safety bound rejects > 8 stages", "[composition_fuzz][permutations]")
{
    std::vector<StageId> big(9);
    for (StageId i = 0; i < 9; ++i)
        big[i] = i;
    auto perms = allPermutations(big);
    REQUIRE(perms.empty());
}

TEST_CASE("evaluateOrdering: pass-through chain produces finite output + N snapshots", "[composition_fuzz][ordering]")
{
    std::vector<StageAdapter> stages = {makePassThrough(0, "A"), makePassThrough(1, "B"), makePassThrough(2, "C")};
    auto r = evaluateOrdering({0, 1, 2}, stages, 1.0);
    REQUIRE(r.produced_finite_output);
    REQUIRE(r.seam_snapshots.size() == 3);
}

TEST_CASE("evaluateOrdering: stage producing NaN flags non-finite output", "[composition_fuzz][finite]")
{
    StageAdapter bad;
    bad.id = 7;
    bad.name = "NaNStage";
    bad.process = [](double) {
        return std::nan("");
    };
    bad.snapshot = []() {
        return CrossStageSnapshot {};
    };
    std::vector<StageAdapter> stages = {bad};
    auto r = evaluateOrdering({7}, stages, 1.0);
    REQUIRE_FALSE(r.produced_finite_output);
}

TEST_CASE("evaluateOrdering: malformed ordering (unknown id) flags non-finite", "[composition_fuzz][ordering]")
{
    std::vector<StageAdapter> stages = {makePassThrough(0, "A")};
    auto r = evaluateOrdering({0, 42 /* unknown */}, stages, 1.0);
    REQUIRE_FALSE(r.produced_finite_output);
}

TEST_CASE("runCompositionFuzz: clean chain + monotonic-timestamp stages passes all permutations",
          "[composition_fuzz][end-to-end]")
{
    // 3 stages each with strictly-monotone-by-id timestamps so any ordering
    // satisfies schedule monotonicity.
    std::vector<StageAdapter> stages;
    for (StageId i = 0; i < 3; ++i)
    {
        auto a = makePassThrough(i, "S" + std::to_string(i), static_cast<std::int64_t>(i) * 1000);
        stages.push_back(std::move(a));
    }
    CrossStageInvariantSet inv;
    inv.registerInvariant("energy", "Parseval", makeEnergyConservationInvariant());

    auto perms = allPermutations({0, 1, 2});
    auto verdict = runCompositionFuzz(perms, stages, inv, 1.0, /*seed=*/42);
    REQUIRE(verdict.passed);
    REQUIRE(verdict.orderings_evaluated == 6);
    REQUIRE(verdict.orderings_failed == 0);
}

TEST_CASE("runCompositionFuzz: deliberate energy-leak stage triggers invariant failure", "[composition_fuzz][red]")
{
    // Two pass-through stages + one deliberately-attenuating stage with
    // NO declared gain factor → energy invariant must catch it on every
    // permutation where the bad stage appears.
    std::vector<StageAdapter> stages;
    stages.push_back(makePassThrough(0, "PassA"));
    stages.push_back(makePassThrough(1, "PassB"));

    StageAdapter leaky;
    leaky.id = 2;
    leaky.name = "Leaky";
    leaky.process = [](double e) {
        return e * 0.5;
    };
    leaky.snapshot = []() {
        CrossStageSnapshot s;
        s.stage_name = "Leaky";
        s.energy_in = 1.0;
        s.energy_out = 0.5; // deliberate leak
        return s;
    };
    stages.push_back(std::move(leaky));

    CrossStageInvariantSet inv;
    inv.registerInvariant("energy", "Parseval", makeEnergyConservationInvariant(1e-6));

    auto perms = allPermutations({0, 1, 2});
    auto verdict = runCompositionFuzz(perms, stages, inv, 1.0, /*seed=*/7);
    REQUIRE_FALSE(verdict.passed);
    REQUIRE(verdict.orderings_failed > 0);
    REQUIRE(verdict.first_failing_ordering.has_value());
    REQUIRE(verdict.first_failure_diagnostic.find("energy drift") != std::string::npos);
}

TEST_CASE("runCompositionFuzz: schedule regression in random ordering caught", "[composition_fuzz][red][schedule]")
{
    // Two stages with conflicting timestamps; some orderings have
    // monotone-forward, others have regression. Invariant must catch
    // the latter.
    auto a_ts100 = makePassThrough(0, "ts100", 100);
    auto a_ts50 = makePassThrough(1, "ts50", 50);
    std::vector<StageAdapter> stages = {a_ts100, a_ts50};

    CrossStageInvariantSet inv;
    inv.registerInvariant("monotonicity", "causal-scheduler property", makeScheduleMonotonicityInvariant());

    auto perms = allPermutations({0, 1});
    auto verdict = runCompositionFuzz(perms, stages, inv, 1.0, /*seed=*/3);
    REQUIRE_FALSE(verdict.passed);
    // Exactly one of two orderings should fail (the {0,1} one with
    // ts100 -> ts50 regression).
    REQUIRE(verdict.orderings_failed == 1);
}

TEST_CASE("runCompositionFuzz: deterministic seeding reproduces failure across runs", "[composition_fuzz][determinism]")
{
    auto a_ts100 = makePassThrough(0, "ts100", 100);
    auto a_ts50 = makePassThrough(1, "ts50", 50);
    std::vector<StageAdapter> stages = {a_ts100, a_ts50};

    CrossStageInvariantSet inv;
    inv.registerInvariant("monotonicity", "causal-scheduler property", makeScheduleMonotonicityInvariant());

    auto perms = allPermutations({0, 1});
    auto v1 = runCompositionFuzz(perms, stages, inv, 1.0, /*seed=*/12345);
    auto v2 = runCompositionFuzz(perms, stages, inv, 1.0, /*seed=*/12345);
    REQUIRE(v1.orderings_evaluated == v2.orderings_evaluated);
    REQUIRE(v1.orderings_failed == v2.orderings_failed);
    REQUIRE(v1.first_failure_diagnostic == v2.first_failure_diagnostic);
}

TEST_CASE("property: clean N-permutation chain remains passing across random seeds", "[composition_fuzz][property]")
{
    auto seed = GENERATE(take(10, random(1u, 100000u)));
    std::vector<StageAdapter> stages;
    for (StageId i = 0; i < 4; ++i)
    {
        stages.push_back(makePassThrough(i, "S" + std::to_string(i), static_cast<std::int64_t>(i) * 1000));
    }
    CrossStageInvariantSet inv;
    inv.registerInvariant("energy", "Parseval", makeEnergyConservationInvariant());
    auto perms = allPermutations({0, 1, 2, 3});
    auto v = runCompositionFuzz(perms, stages, inv, 1.0, seed);
    REQUIRE(v.passed);
    REQUIRE(v.orderings_evaluated == perms.size());
}
