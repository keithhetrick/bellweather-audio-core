// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <functional>
#include <cstdint>

namespace bws::domain
{

/**
 * Driven port: timer / deferred callback scheduling.
 * JUCE-backed implementations live in bw_juce_adapters.
 * Removes Timer inheritance from the scheduler's public headers.
 *
 * Single timer per instance. Calling scheduleRepeating while a timer
 * is active replaces the existing callback.
 */
struct IScheduler
{
    virtual ~IScheduler() = default;
    virtual void scheduleRepeating(uint32_t intervalMs, std::function<void()> callback) = 0;
    virtual void cancel() = 0;
};

} // namespace bws::domain
