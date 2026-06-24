// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

// =============================================================================
// test_rt_smoothed_param - C++ Unit tests
//                          (bws::domain::RtSmoothedParam primitive)
// a, 2026-05-14 (HOLD POINT structure backfilled in slice 15)
// =============================================================================
//
// HOLD POINT 1 - CONTRACT
// -----------------------------------------------------------------------------
// Contract sources:
// - ``
//     industry-survey-driven design rationale (4-agent /research-deep
//     pass; bumpless-transfer convention from control systems;
//     factored-API canon from JUCE post-deprecation; mandatory-
//     initialValue from RxJS BehaviorSubject + reactive programming).
//   - `modules/bw_audio_types/include/bw_audio_types/RtSmoothedParam.h`
//     header docstring - the 3 patterns the type bundles + ResetPolicy
// semantics + header-only constraint.
// - a plan + slice 14b/c migration history (the type's
//     bit-identical-by-construction wrapper migration).
//
// What the contract requires:
//   - **Pattern 1 - sample-accurate linear ramp**: per-block API
// (`setTarget` + `skip` + `currentValue`) wraps ``
//     with the same shape; `setTarget` always ramps; `resetTo` always
//     snaps. No bundled overload (factored per JUCE deprecation canon).
//   - **Pattern 2 - bumpless transfer at prepare()**: `initialValue` is
//     a REQUIRED argument to `prepare()`. There is no way to construct
//     an instance in an unseeded state. Eliminates the seeded-flag
//     class of bug by API contract (Orastron `bw_slew_lim_reset_state`
//     precedent; RxJS `BehaviorSubject<T>` precedent).
//   - **Pattern 3 - reset-persistence policy**: `ResetPolicy` enum is
//     prepare-time documentation of intent (`ColdInit` /
//     `PersistOnReset`). Actual persistence behavior emerges from
//     caller discipline at the reset() site (caller chooses whether
//     to invoke `resetTo()` for a given smoother). Aligns with
//     "generic global, specific local" principle.
//   - `prepare()` may be called multiple times; each call snaps to
//     the new initial value (no stale state).
//   - `isSmoothing()` accurately reflects ramp state (false at
//     prepare, true mid-ramp, false at convergence).
//   - `getNextValue()` advances by one sample per call.
//
// HOLD POINT 2 - ASSERTIONS (regression-net for the primitive contract)
// -----------------------------------------------------------------------------
// POS #1 (Pattern 1 - convergence to target via setTarget+skip):
// - Catches: removal of the underlying `::skip`
//     forwarding; off-by-one in the ramp-length computation.
//
// POS #2 (Pattern 1 - setTarget to same value is a no-op):
//   - Catches: removal of the equality guard in `setTargetValue`;
//     spurious ramp re-arm on identical target.
//
// POS #3 (Pattern 2 - bumpless: first setTarget after prepare(initial=v)
//          starts ramp from v, not from zero):
//   - Catches: removal of the `setCurrentAndTargetValue(initialValue)`
//     call in `prepare()`. This is the load-bearing slice-10b cold-
//     start bug expressed as a unit test - pre-fix behavior would have
//     `currentValue == 0` immediately after prepare; first ramp would
//     crawl from 0 toward target → audible cold-start artifact.
//
// POS #4 (Pattern 2 - multiple prepare() calls each snap fresh):
//   - Catches: stale ramp-state retention across re-prepare; missing
//     reset of `stepsRemaining_` / `step_` in the underlying primitive.
//
// POS #5 (Pattern 3 - resetTo() snaps current and target, cancels ramp):
//   - Catches: incomplete `resetTo` impl that doesn't cancel
//     in-flight ramps; partial state reset.
//
// POS #6 (Pattern 3 - ResetPolicy is documentation of intent; actual
//          persistence is caller-driven by NOT calling resetTo):
//   - Documents the "generic global, specific local" principle: the
//     primitive doesn't enforce policy, the caller's reset() site does.
//
// POS #7 (default policy is ColdInit):
//   - Catches: regression in the default-argument value.
//
// POS #8 (Pattern 1 - getNextValue advances by one sample per call):
//   - Catches: getNextValue not forwarding to underlying primitive's
//     advance; getNextValue returning current without advancing
//     (which was the slice 14b bug we caught with this test).
//
// POS #9 (methodology check - isSmoothing tracks state correctly):
//   - Catches: isSmoothing returning stale value; missing
//     ramp-completion clearing of stepsRemaining_.
//
// Mutation question
//   - Remove `setCurrentAndTargetValue(initialValue)` from prepare() →
//     POS #3 + #4 fail (bumpless invariant broken). Verified live
//     during slice 14a HP3 development.
//   - Replace `getNextValue() { return smoother_.getNextValue(); }` with
//     `return smoother_.getCurrentValue();` (don't advance) → POS #8
//     fails. Verified live during slice 14b development.
//   - Remove the equality guard in setTargetValue → POS #2 fails
//     (smoother re-arms ramp on identical target).
//   - Remove `stepsRemaining_ = 0` from setCurrentAndTargetValue →
//     POS #5 fails (resetTo doesn't cancel in-flight ramp).
//
// Note on test scope: this test exercises `bws::domain::RtSmoothedParam`
// in isolation as a primitive. Production-callsite integration tests
// (the 8 existing rt smoothers + 6 from slice 14c) live in 's
// `tests/dual_mode_smoothing_tests.cpp` (Layer 2 wiring) and
// `tests/knob_transition_smoothing_tests.cpp` (Layer 3 parameter-domain
// for slice 14c surfaces).
// =============================================================================

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <bw_audio_types/RtSmoothedParam.h>

using bws::domain::RtSmoothedParam;
using Catch::Matchers::WithinAbs;

namespace
{
constexpr double kSr = 48000.0;
constexpr double kRampMs = 10.0; // 480 samples
constexpr int kRampSamples = static_cast<int>(kSr * kRampMs * 0.001);
} // namespace

// =============================================================================
// Pattern 1 - sample-accurate linear ramp
// =============================================================================

TEST_CASE("RtSmoothedParam - ramps from initial toward target over rampMs", "[domain][rt_smoothed_param][ramp]")
{
    RtSmoothedParam p;
    p.prepare(kSr, kRampMs, 0.0f);
    REQUIRE(p.currentValue() == 0.0f);
    REQUIRE_FALSE(p.isSmoothing());

    p.setTarget(1.0f);
    REQUIRE(p.isSmoothing());

    // Mid-ramp: value should be between 0 and 1
    p.skip(kRampSamples / 2);
    REQUIRE(p.currentValue() > 0.0f);
    REQUIRE(p.currentValue() < 1.0f);
    REQUIRE(p.isSmoothing());

    // Past full ramp: value should latch at target
    p.skip(kRampSamples);
    REQUIRE_THAT(p.currentValue(), WithinAbs(1.0f, 1e-6f));
    REQUIRE_FALSE(p.isSmoothing());
}

TEST_CASE("RtSmoothedParam - setTarget to same value is a no-op", "[domain][rt_smoothed_param][ramp]")
{
    RtSmoothedParam p;
    p.prepare(kSr, kRampMs, 0.5f);
    REQUIRE_FALSE(p.isSmoothing());

    p.setTarget(0.5f);
    REQUIRE_FALSE(p.isSmoothing()); // no ramp begins
    REQUIRE(p.currentValue() == 0.5f);
}

// =============================================================================
// Pattern 2 - bumpless transfer at prepare()
// =============================================================================

TEST_CASE("RtSmoothedParam - bumpless: first setTarget after prepare(initial=v) "
          "starts ramp from v, not from zero",
          "[domain][rt_smoothed_param][bumpless]")
{
    // This is the slice 10b cold-start bug expressed as a test.
    // Pre-fix behavior would have currentValue == 0 immediately after
    // prepare; first ramp would crawl from 0 toward 5.0 → audible artifact.
    // Post-fix: currentValue == 5.0 immediately; ramp from 5.0 toward 7.0.
    RtSmoothedParam p;
    p.prepare(kSr, kRampMs, 5.0f);
    REQUIRE(p.currentValue() == 5.0f);

    p.setTarget(7.0f);
    // After 0 samples advanced, current is still seeded value (ramp not yet
    // tickled).
    REQUIRE(p.currentValue() == 5.0f);

    p.skip(kRampSamples);
    REQUIRE_THAT(p.currentValue(), WithinAbs(7.0f, 1e-6f));
}

TEST_CASE("RtSmoothedParam - prepare can be called multiple times; each "
          "call snaps to the new initial value (no stale state)",
          "[domain][rt_smoothed_param][bumpless]")
{
    RtSmoothedParam p;
    p.prepare(kSr, kRampMs, 1.0f);
    p.setTarget(3.0f);
    p.skip(kRampSamples / 4); // mid-ramp
    REQUIRE(p.isSmoothing());

    // Second prepare resets ramp state and seeds new initial.
    p.prepare(kSr, kRampMs, -2.0f);
    REQUIRE(p.currentValue() == -2.0f);
    REQUIRE_FALSE(p.isSmoothing());
}

// =============================================================================
// Pattern 3 - reset-persistence policy
// =============================================================================

TEST_CASE("RtSmoothedParam - resetTo() snaps current and target, cancels ramp", "[domain][rt_smoothed_param][reset]")
{
    RtSmoothedParam p;
    p.prepare(kSr, kRampMs, 0.0f);
    p.setTarget(1.0f);
    p.skip(kRampSamples / 2);
    REQUIRE(p.isSmoothing());

    p.resetTo(0.25f);
    REQUIRE(p.currentValue() == 0.25f);
    REQUIRE_FALSE(p.isSmoothing());
}

TEST_CASE("RtSmoothedParam - ResetPolicy is documentation of intent; "
          "actual persistence is caller-driven (skipping resetTo persists)",
          "[domain][rt_smoothed_param][reset][policy]")
{
    // Caller's intent: this smoother tracks audible reality (e.g.,
    // autoMakeupLiteDb). Caller's reset() should NOT call resetTo() - value
    // persists. The primitive does not enforce this; it documents the intent.
    RtSmoothedParam p;
    p.prepare(kSr, kRampMs, 3.5f, RtSmoothedParam::ResetPolicy::PersistOnReset);
    p.setTarget(4.0f);
    p.skip(kRampSamples);
    REQUIRE_THAT(p.currentValue(), WithinAbs(4.0f, 1e-6f));

    // Caller chooses NOT to reset (because policy says PersistOnReset).
    // Value persists across whatever lifecycle event triggered the reset.
    REQUIRE(p.policy() == RtSmoothedParam::ResetPolicy::PersistOnReset);
    REQUIRE_THAT(p.currentValue(), WithinAbs(4.0f, 1e-6f));
}

TEST_CASE("RtSmoothedParam - default policy is ColdInit", "[domain][rt_smoothed_param][policy]")
{
    RtSmoothedParam p;
    p.prepare(kSr, kRampMs, 0.0f);
    REQUIRE(p.policy() == RtSmoothedParam::ResetPolicy::ColdInit);
}

// =============================================================================
// Negative / methodology guards
// =============================================================================

TEST_CASE("RtSmoothedParam - getNextValue advances by one sample per call", "[domain][rt_smoothed_param][per_sample]")
{
    // Per-sample API contract - used by callers like GainUtils that loop
    // sample-by-sample. Each call advances ramp by 1 step. b
    // addition (the 14a primitive omitted it; the SampleAccurateSmoother
    // wrapper needs it for bit-identical behavior).
    RtSmoothedParam p;
    p.prepare(kSr, kRampMs, 0.0f);
    p.setTarget(1.0f);

    const float v0 = p.getNextValue(); // step 1
    const float v1 = p.getNextValue(); // step 2
    REQUIRE(v0 > 0.0f);
    REQUIRE(v1 > v0);

    // Drain the rest of the ramp; final value latches at target.
    for (int i = 0; i < kRampSamples; ++i)
    {
        (void)p.getNextValue();
    }
    REQUIRE_THAT(p.currentValue(), WithinAbs(1.0f, 1e-6f));
    REQUIRE_FALSE(p.isSmoothing());
}

TEST_CASE("RtSmoothedParam - isSmoothing methodology check: false at "
          "prepare, true mid-ramp, false at convergence",
          "[domain][rt_smoothed_param][methodology]")
{
    RtSmoothedParam p;
    p.prepare(kSr, kRampMs, 0.0f);
    REQUIRE_FALSE(p.isSmoothing());

    p.setTarget(1.0f);
    REQUIRE(p.isSmoothing());

    p.skip(kRampSamples / 4);
    REQUIRE(p.isSmoothing());

    p.skip(kRampSamples);
    REQUIRE_FALSE(p.isSmoothing());
}
