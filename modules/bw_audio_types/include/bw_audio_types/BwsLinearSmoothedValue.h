// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <algorithm>
#include <cmath>

namespace bws::domain
{

/**
 * Linear parameter smoother for sample-counted ramps.
 * Ramps from current value to target over a fixed time window.
 * Real-time safe: no allocations, no locks.
 *
 * API shape:
 *   reset(sampleRate, rampLengthSeconds)
 *   setCurrentAndTargetValue(v)
 *   setTargetValue(v)
 *   getNextValue()
 *   isSmoothing()
 */
class BwsLinearSmoothedValue
{
public:
    explicit BwsLinearSmoothedValue(float initialValue = 0.0f) noexcept
        : current_(initialValue)
        , target_(initialValue)
    {}

    /// Configure ramp length. Call before audio processing starts (not RT-safe).
    void reset(double sampleRate, double rampLengthSeconds) noexcept
    {
        rampLength_ = std::max(1, static_cast<int>(std::round(sampleRate * rampLengthSeconds)));
        stepsRemaining_ = 0;
        step_ = 0.0f;
    }

    /// Set both current and target - no ramp, instant jump.
    void setCurrentAndTargetValue(float value) noexcept
    {
        current_ = value;
        target_ = value;
        stepsRemaining_ = 0;
        step_ = 0.0f;
    }

    /// Set target - ramp begins on next getNextValue() call.
    /// If target has not changed, the existing ramp continues uninterrupted.
    void setTargetValue(float target) noexcept
    {
        if (!std::islessgreater(target, target_))
            return;
        target_ = target;
        stepsRemaining_ = rampLength_;
        step_ = (target_ - current_) / static_cast<float>(stepsRemaining_);
    }

    /// Advance one sample and return the smoothed value.
    float getNextValue() noexcept
    {
        if (stepsRemaining_ <= 0)
            return current_;
        current_ += step_;
        --stepsRemaining_;
        if (stepsRemaining_ == 0)
            current_ = target_; // clamp drift
        return current_;
    }

    /// Advance by N samples, return the final value. RT-safe.
    float skip(int numSamples) noexcept
    {
        if (stepsRemaining_ <= 0)
            return current_;
        if (numSamples >= stepsRemaining_)
        {
            current_ = target_;
            stepsRemaining_ = 0;
            step_ = 0.0f;
        }
        else
        {
            current_ += step_ * static_cast<float>(numSamples);
            stepsRemaining_ -= numSamples;
        }
        return current_;
    }

    bool isSmoothing() const noexcept { return stepsRemaining_ > 0; }
    float getCurrentValue() const noexcept { return current_; }
    float getTargetValue() const noexcept { return target_; }

private:
    float current_ {0.0f};
    float target_ {0.0f};
    float step_ {0.0f};
    int stepsRemaining_ {0};
    int rampLength_ {1};
};

} // namespace bws::domain
