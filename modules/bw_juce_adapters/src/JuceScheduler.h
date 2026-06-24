// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bw_ports/IScheduler.h>
#include <juce_events/juce_events.h>
#include <functional>
#include <cstdint>

namespace bws::adapters
{

/**
 * JUCE adapter for the IScheduler port.
 * Wraps juce::Timer - the only place in the codebase
 * where juce::Timer is permitted to exist.
 *
 * Lives in bw_juce_adapters. Keep this out of core modules,
 * any module above the adapter layer.
 */
class JuceScheduler final
    : public bws::domain::IScheduler
    , private juce::Timer
{
public:
    JuceScheduler() = default;
    ~JuceScheduler() override { cancel(); }

    void scheduleRepeating(uint32_t intervalMs, std::function<void()> callback) override
    {
        callback_ = std::move(callback);
        startTimer(static_cast<int>(intervalMs));
    }

    void cancel() override
    {
        stopTimer();
        callback_ = nullptr;
    }

private:
    void timerCallback() override
    {
        if (callback_)
            callback_();
    }

    std::function<void()> callback_;
};

} // namespace bws::adapters
