// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_toy_shell/TimerPump.h"

#include <algorithm>

namespace bws::toy_shell
{

namespace
{
constexpr int kTimerIntervalMs = 16; // ~60Hz
constexpr double kMaxDtSeconds = 0.05;
} // namespace

TimerPump::TimerPump(std::function<void(double)> onTick)
    : onTick_(std::move(onTick))
{}

void TimerPump::start()
{
    if (isTimerRunning())
        return; // idempotent - preserves the in-flight dt clock
    lastTickMs_ = juce::Time::getMillisecondCounterHiRes();
    startTimer(kTimerIntervalMs);
}

void TimerPump::stop()
{
    stopTimer(); // juce::Timer::stopTimer is safe on a never-started instance
}

void TimerPump::timerCallback()
{
    const double nowMs = juce::Time::getMillisecondCounterHiRes();
    const double dt = std::min(kMaxDtSeconds, (nowMs - lastTickMs_) / 1000.0);
    lastTickMs_ = nowMs;
    if (onTick_)
        onTick_(dt);
}

} // namespace bws::toy_shell
