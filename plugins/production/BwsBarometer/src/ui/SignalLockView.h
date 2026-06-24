// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../toy/SignalLockTypes.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <cstddef>

namespace bws::barometer::toy
{

/// Oscilloscope paint of the Signal Lock sim. Reads `SignalLockState` via an
/// externally-provided pointer; renders into its own bounds in `paint()`.
/// Holds a small renderer-local ring buffer of recent `state.error` samples
/// (the rolling 5-second error scope drawn in the bottom-right corner).
/// Callers push samples via `pushErrorSample` from the same timer that drives
/// `updateSignalLock`.
class SignalLockView : public juce::Component
{
public:
    /// Capacity of the rolling-error ring buffer (~5 seconds at 30 Hz).
    static constexpr std::size_t kErrorHistoryCapacity = 150;

    SignalLockView();

    /// Non-owning. The state must outlive every paint() call after this.
    void setSignalLockState(const SignalLockState* state) noexcept { state_ = state; }

    void setScale(float scale) noexcept { scale_ = juce::jmax(0.25f, scale); }

    /// Append a new error sample to the ring buffer. The renderer reads
    /// samples chronologically in `paint()` regardless of buffer wrap state.
    void pushErrorSample(double error) noexcept;

    void paint(juce::Graphics& g) override;

private:
    const SignalLockState* state_ = nullptr;
    std::array<double, kErrorHistoryCapacity> errorHistory_ {};
    std::size_t errorCursor_ = 0;
    bool errorFilled_ = false;
    float scale_ = 1.0f;
};

} // namespace bws::barometer::toy
