// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once

/// @file
/// @brief Framerate-independent display ballistics for UI-rate meter easing.

#include <cmath>

namespace bws::audio
{

// dt-parameterized display ballistics for UI-rate meter easing.
// Framerate-independent: results depend only on elapsed time and the time
// constant, never on how often the clock ticks (alpha = 1 - exp(-dt / tau)).
struct MeterBallistics
{
    // One-pole blend coefficient toward a target over dtSeconds with time
    // constant timeMs. dt <= 0 holds (0); timeMs <= 0 snaps (1).
    static float onePoleCoeff(float timeMs, float dtSeconds) noexcept
    {
        if (dtSeconds <= 0.0f)
            return 0.0f;
        const float tauSeconds = timeMs * 0.001f;
        if (tauSeconds <= 0.0f)
            return 1.0f;
        return 1.0f - std::exp(-dtSeconds / tauSeconds);
    }

    // Ease state toward target over dtSeconds with a single time constant.
    static float easeToward(float state, float target, float timeMs, float dtSeconds) noexcept
    {
        return state + onePoleCoeff(timeMs, dtSeconds) * (target - state);
    }

    // Asymmetric ease: attackMs while target rises above state, releaseMs while
    // it falls. (Level meters: louder = rising = attack.)
    static float attackRelease(float state, float target, float attackMs, float releaseMs, float dtSeconds) noexcept
    {
        const float timeMs = (target > state) ? attackMs : releaseMs;
        return easeToward(state, target, timeMs, dtSeconds);
    }

    // Fall toward target at a fixed rate (units/second), clamped at target.
    // dt<=0 or rate<=0 holds; target above state snaps up.
    static float linearFallToward(float state, float target, float ratePerSec, float dtSeconds) noexcept
    {
        if (dtSeconds <= 0.0f || ratePerSec <= 0.0f)
            return state;
        if (target >= state)
            return target;
        const float next = state - (ratePerSec * dtSeconds);
        return (next < target) ? target : next;
    }
};

// Time-based peak hold: holds a captured peak for holdSeconds of elapsed time,
// then eases toward the live value with decayMs. The window is measured in
// seconds, so the hold lasts the same wall-time at any clock rate.
struct PeakHold
{
    float value {0.0f};
    float holdRemainingSeconds {0.0f};

    void update(float target, float holdSeconds, float decayMs, float dtSeconds) noexcept
    {
        if (target >= value)
        {
            value = target;
            holdRemainingSeconds = holdSeconds;
        }
        else if (holdRemainingSeconds > 0.0f)
        {
            holdRemainingSeconds -= dtSeconds;
        }
        else
        {
            value = MeterBallistics::easeToward(value, target, decayMs, dtSeconds);
        }
    }

    // Same hold semantics, but falls linearly at fallPerSec after the hold
    // expires instead of easing exponentially.
    void updateLinear(float target, float holdSeconds, float fallPerSec, float dtSeconds) noexcept
    {
        if (target >= value)
        {
            value = target;
            holdRemainingSeconds = holdSeconds;
        }
        else if (holdRemainingSeconds > 0.0f)
        {
            holdRemainingSeconds -= dtSeconds;
        }
        else
        {
            value = MeterBallistics::linearFallToward(value, target, fallPerSec, dtSeconds);
        }
    }
};

} // namespace bws::audio
