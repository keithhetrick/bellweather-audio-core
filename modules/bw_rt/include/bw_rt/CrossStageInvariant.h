// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

/// @file
/// @brief Registration interface for cross-stage runtime-probe invariants between DSP stages.

// =============================================================================
// CrossStageInvariant.h - narrow registration interface for cross-stage
// runtime-probe invariants.
//
// Catches the bug class where each DSP stage in isolation passes its unit
// tests but the cooperation BETWEEN stages drifts (latency upstream-reports
// != downstream-consumes; schedule timestamps regress; energy not conserved
// across a seam).
//
// Interface contract:
//
//   1. Caller captures a CrossStageSnapshot at each side of the seam.
//   2. Caller registers one InvariantFn per cross-stage law to enforce.
//   3. Test driver calls evaluateAll(before, after); receives first
//      failing invariant name + diagnostic, or pass.
//
// Framework-neutral by design (lives in bw_rt). Invariant probes that need
// plugin-shell access live in plugins/.../tests/invariant/ and consume
// this header.
//
// Every invariant derives from closed-form math, a published spec, or a
// documented system property. Each registration is paired with a comment
// block citing the derivation source.
// =============================================================================

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace bws::rt
{

// Snapshot of the state captured at one side of a cross-stage seam.
// Plain C++ types only, so this header is consumable from any
// module. Stages populate the fields they expose; an unset field carries
// a sentinel (NaN / std::nullopt) and invariants must handle that
// explicitly.
struct CrossStageSnapshot
{
    std::string stage_name;                      // e.g. "LookaheadStage", "CrossfadeScheduler"
    std::uint64_t block_index {0};               // monotonically increasing per processBlock
    std::int64_t schedule_timestamp_samples {0}; // monotone schedule clock
    double latency_reported_samples {0.0};       // upstream-reported latency
    double latency_consumed_samples {0.0};       // downstream-consumed latency
    double energy_in {0.0};                      // sum-of-squares input
    double energy_out {0.0};                     // sum-of-squares output
    double sample_rate_hz {0.0};
    std::size_t channel_count {0};
};

// Invariant evaluation result. Pass → empty diagnostic. Fail → diagnostic
// carries human-readable explanation suitable for direct printing in a
// Catch2 INFO() block.
struct InvariantResult
{
    bool passed {true};
    std::string diagnostic;
};

// Predicate signature: (before-seam, after-seam) → result.
// Deliberately a free function signature (not a class) so tests can
// compose lambdas / capture closed-form constants without subclassing.
using InvariantFn = std::function<InvariantResult(const CrossStageSnapshot& before, const CrossStageSnapshot& after)>;

struct RegisteredInvariant
{
    std::string name;       // short identifier; printed on failure
    std::string derivation; // citation: closed-form / spec / property source
    InvariantFn fn;
};

// Aggregate evaluation result. If any invariant failed, the first
// failure is captured in `first_failure`. All other failures are
// reported via `all_failures` for debugging.
struct AggregateResult
{
    bool passed {true};
    std::optional<std::string> first_failure_name;
    std::string first_failure_diagnostic;
    std::vector<std::string> all_failures;
};

// Container holding registered invariants. Tests construct one, register
// the per-seam invariant set, then call evaluateAll() per
// (before, after) snapshot pair.
//
// Not thread-safe - invariants are evaluated synchronously at test time
// against captured snapshots. Snapshots themselves are captured on the
// audio thread (typically into a lock-free buffer) and consumed by the
// test driver thread.
class CrossStageInvariantSet
{
public:
    CrossStageInvariantSet() = default;

    void registerInvariant(std::string name, std::string derivation, InvariantFn fn)
    {
        invariants_.push_back({std::move(name), std::move(derivation), std::move(fn)});
    }

    [[nodiscard]] AggregateResult evaluateAll(const CrossStageSnapshot& before, const CrossStageSnapshot& after) const
    {
        AggregateResult agg;
        for (const auto& inv : invariants_)
        {
            auto r = inv.fn(before, after);
            if (!r.passed)
            {
                if (!agg.first_failure_name.has_value())
                {
                    agg.first_failure_name = inv.name;
                    agg.first_failure_diagnostic = r.diagnostic;
                }
                agg.all_failures.push_back(inv.name + ": " + r.diagnostic);
                agg.passed = false;
            }
        }
        return agg;
    }

    [[nodiscard]] std::size_t size() const noexcept { return invariants_.size(); }

    [[nodiscard]] std::string_view nameAt(std::size_t i) const { return invariants_.at(i).name; }

private:
    std::vector<RegisteredInvariant> invariants_;
};

// -----------------------------------------------------------------------------
// Reusable invariant primitives - pure functions derived from
// closed-form math. Tests compose these into a CrossStageInvariantSet.
//
// Each primitive's derivation is in its docstring; reviewers verify the
// derivation matches the cited source, NOT that it makes the current code quiet
// (bias-resistance discipline).
// -----------------------------------------------------------------------------

// Latency consistency: upstream-reported latency MUST equal downstream-
// consumed latency at each block. Tolerance handles fractional-delay
// rounding (sub-sample) where applicable.
//
// Derivation: from the host latency-compensation contract. Cross-stage
// equivalent: the consumer of a latency-reporting stage must consume exactly
// what is reported. Discrepancy = phase drift = audible artifact.
inline InvariantFn makeLatencyConsistencyInvariant(double tolerance_samples = 0.5)
{
    return [tolerance_samples](const CrossStageSnapshot& before, const CrossStageSnapshot& after) -> InvariantResult {
        const double diff = before.latency_reported_samples - after.latency_consumed_samples;
        const double abs_diff = diff < 0.0 ? -diff : diff;
        if (abs_diff > tolerance_samples)
        {
            return {false, "latency mismatch: " + before.stage_name + " reports " +
                               std::to_string(before.latency_reported_samples) + " samples; " + after.stage_name +
                               " consumes " + std::to_string(after.latency_consumed_samples) + " samples (tolerance " +
                               std::to_string(tolerance_samples) + ")"};
        }
        return {};
    };
}

// Schedule monotonicity: schedule timestamps MUST never regress between
// adjacent seam snapshots. Equal is acceptable (same block); strict
// regression is not.
//
// Derivation: any scheduler producing event timestamps consumed
// downstream must satisfy t_{n+1} >= t_n (causal). Regression = the
// downstream consumer would re-process a past schedule = duplicate
// fire or undefined behavior.
inline InvariantFn makeScheduleMonotonicityInvariant()
{
    return [](const CrossStageSnapshot& before, const CrossStageSnapshot& after) -> InvariantResult {
        if (after.schedule_timestamp_samples < before.schedule_timestamp_samples)
        {
            return {false, "schedule regression: " + before.stage_name + " @ " +
                               std::to_string(before.schedule_timestamp_samples) + " -> " + after.stage_name + " @ " +
                               std::to_string(after.schedule_timestamp_samples)};
        }
        return {};
    };
}

// Energy conservation within epsilon: sum-of-squares energy at the
// output should be within epsilon of input energy, scaled by the
// stage's known gain factor. Default gain_factor = 1.0 (passive seam
// expected to neither add nor remove energy beyond round-off).
//
// Derivation: Parseval's theorem (FFT-domain) implies discrete-time
// energy preservation across any linear, lossless, unity-gain transform.
// Discrepancy beyond round-off = unintended attenuation, denormal flush
// of significant signal, or composition bug introducing extra gain
// stages.
//
// epsilon_relative: fractional tolerance (e.g., 1e-6 = ~120dB headroom).
// Tests on lossy stages (compressors, limiters) MUST pass a higher
// epsilon OR an explicit gain_factor derived from the static gain
// reduction at the test stimulus level.
inline InvariantFn makeEnergyConservationInvariant(double epsilon_relative = 1e-6, double gain_factor = 1.0)
{
    return [epsilon_relative, gain_factor](const CrossStageSnapshot& before,
                                           const CrossStageSnapshot& after) -> InvariantResult {
        const double expected = before.energy_out * gain_factor;
        const double actual = after.energy_out;
        const double denom = expected > 1e-30 ? expected : 1e-30;
        const double rel = (actual - expected) / denom;
        const double abs_rel = rel < 0.0 ? -rel : rel;
        if (abs_rel > epsilon_relative)
        {
            return {false, "energy drift across seam " + before.stage_name + " -> " + after.stage_name + ": expected " +
                               std::to_string(expected) + " actual " + std::to_string(actual) + " (rel " +
                               std::to_string(rel) + ", tolerance " + std::to_string(epsilon_relative) + ")"};
        }
        return {};
    };
}

} // namespace bws::rt
