// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace bws::toy_shell
{

/// Wraps juce::Timer to deliver a dt-clamped tick to a registered callback.
/// Matches animation-loop hosts: dt is clamped to
/// [0, 0.05] seconds so tab-switches / dropped frames don't spike the sim.
class TimerPump : private juce::Timer
{
public:
    explicit TimerPump(std::function<void(double)> onTick);

    /// Idempotent: calling start() on an already-running pump is a no-op
    /// (does NOT reset the dt clock).
    void start();

    /// Idempotent: stop() on a never-started or already-stopped pump is safe.
    void stop();

private:
    void timerCallback() override;

    std::function<void(double)> onTick_;
    double lastTickMs_ = 0.0;
};

} // namespace bws::toy_shell
