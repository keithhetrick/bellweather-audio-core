// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "bw_ui/kernel/Clock.h"

#include <juce_events/juce_events.h>

namespace bws::ui::adapters
{

// Elapsed seconds between two wall-clock reads, clamped. A prevSeconds of 0 is
// the unseeded first call (returns 0); a backwards step returns 0; the clamp
// caps a single step (first frame, debugger pause, app suspend) at maxStep.
[[nodiscard]] inline double clampedDeltaSeconds(double prevSeconds, double nowSeconds, double maxStep = 0.1) noexcept
{
    if (prevSeconds <= 0.0)
        return 0.0;
    const double dt = nowSeconds - prevSeconds;
    if (dt <= 0.0)
        return 0.0;
    return (dt > maxStep) ? maxStep : dt;
}

// One juce::Timer driving the framework-neutral kernel::Clock. Subscribers register a
// rate; the timer ticks at tickHz and the clock fans out per-subscriber.
class JuceTimerClock : private juce::Timer
{
public:
    explicit JuceTimerClock(int tickHz = 60) { startTimerHz(tickHz); }
    ~JuceTimerClock() override { stopTimer(); }

    void subscribe(kernel::ITicker* ticker, double hz) { clock_.subscribe(ticker, hz); }
    void unsubscribe(kernel::ITicker* ticker) { clock_.unsubscribe(ticker); }
    [[nodiscard]] std::size_t subscriberCount() const { return clock_.subscriberCount(); }

private:
    void timerCallback() override
    {
        const double now = juce::Time::getMillisecondCounterHiRes() * 0.001;
        clock_.advance(clampedDeltaSeconds(prevSeconds_, now));
        prevSeconds_ = now;
    }

    kernel::Clock clock_;
    double prevSeconds_ {0.0};
};

} // namespace bws::ui::adapters
