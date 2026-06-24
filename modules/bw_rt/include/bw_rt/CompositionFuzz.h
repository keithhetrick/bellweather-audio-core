// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

/// @file
/// @brief Random-permutation harness for cross-stage DSP cooperation testing.

// =============================================================================
// CompositionFuzz.h - random permutation harness for cross-stage cooperation.
//
// Catches the bug class where each DSP stage is correct in isolation AND each
// adjacent-stage pair is correct, but a random reordering of the safe-
// permutation set surfaces a composition bug (state leakage across an
// unexpected adjacency; non-finite output; energy/latency invariant
// violation across the full chain).
//
// Interface contract: narrow, framework-neutral, deterministic.
//
//   1. Caller declares a StageId enum + a safe-permutation set
//      (orderings that should produce equivalent output within epsilon).
//   2. Caller supplies a Stage struct: id + process(input) -> output +
//      snapshot() -> CrossStageSnapshot.
//   3. Caller registers cross-stage invariants via CrossStageInvariantSet.
//   4. Harness exhaustively enumerates the safe-permutation set, runs each
//      ordering through the registered invariants, surfaces any failures.
//   5. Non-finite output on any ordering = automatic failure.
//
// Deterministic seeding via std::mt19937 + explicit seed_seq so failures
// reproduce across runs and platforms (cross-arch fuzz parity).
//
// The caller documents each safe-permutation set from closed-form math, a
// published spec, or domain law. If the harness reports
// "ordering X violates invariant Y", the failure is measured against that
// contract.
// =============================================================================

#include <algorithm>
#include <bw_rt/CrossStageInvariant.h>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <random>
#include <string>
#include <string_view>
#include <vector>

namespace bws::rt
{

// Opaque per-stage handle. Caller defines the concrete enum/type;
// harness operates on uint32_t identifiers so the header stays framework-neutral.
// and dependency-free.
using StageId = std::uint32_t;

// One ordering of stages. Empty vector = identity (no stages).
using StageOrdering = std::vector<StageId>;

// A "safe-permutation set" is the closed enumeration of orderings the
// caller asserts SHOULD produce equivalent output within epsilon. The
// harness checks each ordering against the registered invariants + the
// caller-supplied equivalence predicate.
using SafePermutationSet = std::vector<StageOrdering>;

// Process result emitted by one ordering.
struct CompositionResult
{
    StageOrdering ordering;
    bool produced_finite_output {true};
    std::vector<CrossStageSnapshot> seam_snapshots; // one per stage exit
};

// Top-level fuzz outcome.
struct FuzzVerdict
{
    bool passed {true};
    std::size_t orderings_evaluated {0};
    std::size_t orderings_failed {0};
    std::optional<StageOrdering> first_failing_ordering;
    std::string first_failure_diagnostic;
    std::vector<std::string> all_failures;
};

// Stage adapter - caller's concrete stage exposes these closures so the
// harness can drive it without knowing the stage's storage / framework.
struct StageAdapter
{
    StageId id {};
    std::string name;
    // process(input_energy) -> output_energy; signed-double avoids float
    // FMA ambiguity at the harness boundary.
    std::function<double(double)> process;
    // snapshot() captures the seam state after the most recent process().
    std::function<CrossStageSnapshot(void)> snapshot;
    // Optional reset between orderings (clears stateful filters / delays).
    std::function<void(void)> reset;
};

// Per-permutation evaluation driver.
//
// Walks `ordering` start→end, threading energy through each stage's
// process() closure, calling snapshot() after each, evaluating
// `invariants.evaluateAll(snapshot[i], snapshot[i+1])` between adjacent
// snapshot pairs.
//
// Returns CompositionResult capturing snapshots + finite-check.
inline CompositionResult evaluateOrdering(const StageOrdering& ordering, const std::vector<StageAdapter>& stages,
                                          double input_energy)
{
    CompositionResult result;
    result.ordering = ordering;
    result.seam_snapshots.reserve(ordering.size());

    double energy = input_energy;
    for (StageId id : ordering)
    {
        // Look up stage by id; if missing the ordering is malformed.
        auto it = std::find_if(stages.begin(), stages.end(), [id](const StageAdapter& s) { return s.id == id; });
        if (it == stages.end())
        {
            result.produced_finite_output = false;
            return result;
        }
        if (it->reset)
            it->reset();
        energy = it->process(energy);
        if (!std::isfinite(energy))
        {
            result.produced_finite_output = false;
            return result;
        }
        result.seam_snapshots.push_back(it->snapshot());
    }
    return result;
}

// Top-level fuzz driver.
//
// For each ordering in the safe-permutation set:
//   - evaluate the ordering (threading input_energy through stages)
//   - assert produced_finite_output
//   - for each adjacent (snapshot[i], snapshot[i+1]) pair, run the
//     registered cross-stage invariants
//   - collect failures
//
// Determinism: caller supplies a uint32_t seed used to permute the
// evaluation order of the safe-permutation set itself (so test runs
// surface failures in different orders across seeds - exposes harness
// bias against ordering-of-discovery).
inline FuzzVerdict runCompositionFuzz(const SafePermutationSet& safe_set, const std::vector<StageAdapter>& stages,
                                      const CrossStageInvariantSet& invariants, double input_energy, std::uint32_t seed)
{
    FuzzVerdict verdict;

    // Permute evaluation order deterministically by seed.
    std::vector<std::size_t> indices(safe_set.size());
    for (std::size_t i = 0; i < indices.size(); ++i)
        indices[i] = i;
    std::mt19937 rng(seed);
    std::shuffle(indices.begin(), indices.end(), rng);

    for (std::size_t idx : indices)
    {
        const auto& ordering = safe_set[idx];
        ++verdict.orderings_evaluated;
        auto cr = evaluateOrdering(ordering, stages, input_energy);

        // Finite-output check.
        if (!cr.produced_finite_output)
        {
            ++verdict.orderings_failed;
            verdict.passed = false;
            const std::string msg = "non-finite output for ordering of size " + std::to_string(ordering.size());
            if (!verdict.first_failing_ordering.has_value())
            {
                verdict.first_failing_ordering = ordering;
                verdict.first_failure_diagnostic = msg;
            }
            verdict.all_failures.push_back(msg);
            continue;
        }

        // Adjacent-pair invariant evaluation.
        for (std::size_t i = 0; i + 1 < cr.seam_snapshots.size(); ++i)
        {
            auto agg = invariants.evaluateAll(cr.seam_snapshots[i], cr.seam_snapshots[i + 1]);
            if (!agg.passed)
            {
                ++verdict.orderings_failed;
                verdict.passed = false;
                const std::string msg = "ordering[" + std::to_string(idx) + "] seam " + std::to_string(i) + "->" +
                                        std::to_string(i + 1) + ": " + agg.first_failure_diagnostic;
                if (!verdict.first_failing_ordering.has_value())
                {
                    verdict.first_failing_ordering = ordering;
                    verdict.first_failure_diagnostic = msg;
                }
                verdict.all_failures.push_back(msg);
                break; // first failure per ordering is enough for triage
            }
        }
    }
    return verdict;
}

// Generator helper: produce all N! permutations of a stage set as the
// trivially-safe set (caller asserts ALL permutations should be
// equivalent, e.g., for a chain of commutative-by-design stages).
//
// Refuses N > 8 to bound runtime (8! = 40320; 9! = 362880).
inline SafePermutationSet allPermutations(const std::vector<StageId>& stage_ids)
{
    SafePermutationSet out;
    if (stage_ids.size() > 8)
        return out; // safety bound
    auto sorted = stage_ids;
    std::sort(sorted.begin(), sorted.end());
    do
    {
        out.push_back(sorted);
    } while (std::next_permutation(sorted.begin(), sorted.end()));
    return out;
}

} // namespace bws::rt
