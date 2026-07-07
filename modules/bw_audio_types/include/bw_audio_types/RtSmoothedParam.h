// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

/// @file
/// @brief Sample-accurate, framework-neutral parameter smoother for real-time audio.

#include "BwsLinearSmoothedValue.h"
#include <cstdint>

namespace bws::domain
{

// =============================================================================
// RtSmoothedParam - canonical sample-accurate parameter smoother for
// real-time audio. Header-only and framework-neutral.
//
// Wraps BwsLinearSmoothedValue with three lessons baked into the API:
//
//   1. Sample-accurate linear ramp; per-block setTarget + skip; per-sample
//      getNextValue.
//   2. Bumpless transfer - initial value is REQUIRED at prepare(). Eliminates
//      the seeded-flag class of bug by making the type uninstantiable in an
//      indeterminate state. Term + pattern: 50-year control-systems
//      convention; Orastron bw_slew_lim_reset_state takes initial as a
//      required arg; RxJS BehaviorSubject mandates initial value at
//      construction. (b lesson, generalized.)
//   3. Reset-persistence policy - caller declares intent at prepare-time
//      via ResetPolicy. Actual persistence behavior emerges from caller
//      discipline (caller chooses whether to call resetTo() for a given
//      smoother). Aligns with "generic global, specific local". The enum
//      documents intent for downstream tooling.
//
// Per-block API (RT-safe - no allocation, no locks, no syscalls):
//   setTarget(v)        - update target; ramp begins on next sample
//   skip(n)             - advance N samples, last value latched in current
//   currentValue()      - read smoothed value at consumer site
//   isSmoothing()       - true if a ramp is in progress
//
// Lifecycle API (prepareToPlay-time; alloc-free but conventionally not RT):
//   prepare(sr, ms, initialValue, policy)
//   resetTo(value)      - bumpless re-seed; caller declares new starting point
//
// Industry survey:
// Promotion rationale:
// =============================================================================

class RtSmoothedParam
{
public:
    enum class ResetPolicy : std::uint8_t
    {
        ColdInit,       // Default: caller's reset() should call resetTo() to
                        // re-seed (e.g., per-block parameter smoothers).
        PersistOnReset, // EMA-like state: caller's reset() should SKIP this
                        // smoother (e.g., autoMakeupLiteDb-style trackers).
    };

    RtSmoothedParam() = default;

    // Configure ramp and seed initial value. initialValue is REQUIRED - there
    // is no way to leave the smoother in an unseeded state. Call from
    // prepareToPlay or equivalent lifecycle hook (alloc-free but not
    // conventionally RT-safe).
    void prepare(double sampleRate, double smoothingTimeMs, float initialValue,
                 ResetPolicy policy = ResetPolicy::ColdInit) noexcept
    {
        smoother_.reset(sampleRate, smoothingTimeMs * 0.001);
        smoother_.setCurrentAndTargetValue(initialValue);
        policy_ = policy;
    }

    // Bumpless re-seed - snap current and target to value, cancel any
    // in-flight ramp. Caller declares the new starting point explicitly.
    // For PersistOnReset smoothers, caller's reset() typically does NOT
    // invoke this (the value persists across reset).
    void resetTo(float value) noexcept { smoother_.setCurrentAndTargetValue(value); }

    // Per-block API - RT-safe.
    void setTarget(float value) noexcept { smoother_.setTargetValue(value); }

    // Advance N samples. Underlying skip() returns the last-sample value;
    // we discard it (callers read via currentValue() at the consumer site).
    void skip(int numSamples) noexcept { (void)smoother_.skip(numSamples); }

    // Advance one sample and return the smoothed value. RT-safe.
    // Use this in per-sample processing loops; for per-block advance use
    // skip(numSamples) + currentValue().
    float getNextValue() noexcept { return smoother_.getNextValue(); }

    [[nodiscard]] float currentValue() const noexcept { return smoother_.getCurrentValue(); }

    [[nodiscard]] bool isSmoothing() const noexcept { return smoother_.isSmoothing(); }

    [[nodiscard]] ResetPolicy policy() const noexcept { return policy_; }

private:
    BwsLinearSmoothedValue smoother_;
    ResetPolicy policy_ {ResetPolicy::ColdInit};
};

} // namespace bws::domain
