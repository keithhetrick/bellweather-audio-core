// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include <vector>
#include <atomic>
#include <cstdint>

namespace bws::adapters
{

//==============================================================================
// ValueTreeRetainer - containment with budget guardrails
//
// Prevents destruction of ValueTree SharedObjects that contain JUCE Strings
// bound to a cross-module constexpr emptyString.  Keeps old states alive by
// holding a reference (refcount > 0) so the SharedObject is never freed.
//
// Budget tracking:
//   retainedCount()          - number of retained states
//   retainedEstimatedBytes() - rough byte estimate (properties + children)
//
// Soft warning threshold: logs jassertfalse once when exceeded.
// Hard threshold: still retains (crash prevention trumps budget), but
//   increments a breach counter observable via hardBreachCount().
//
// Thread safety: retain() is NOT thread-safe - call only from the message
// thread (same thread that owns APVTS lifecycle).
//==============================================================================

class ValueTreeRetainer
{
public:
    static ValueTreeRetainer& instance() noexcept
    {
        // Intentional leak - must outlive all plugin instances.
        static auto* inst = new ValueTreeRetainer();
        return *inst;
    }

    /// Disable retention for test harnesses (single-binary, no cross-module
    /// emptyString issue). Call ONCE from main() before any processor creation.
    /// NOT thread-safe - message thread only.
    // /
    static void disableForTesting() noexcept { testingMode_ = true; }
    static bool isTestingMode() noexcept { return testingMode_; }

    void retain(const juce::ValueTree& state)
    {
        if (!state.isValid())
            return;

        retained_.push_back(state);

        const auto count = retained_.size();
        retainedCount_.store(static_cast<uint64_t>(count), std::memory_order_relaxed);

        // Rough byte estimate: each property ~64 bytes (Identifier + var),
        // each child ~128 bytes (SharedObject overhead + properties).
        const auto estBytes = estimateBytes(state);
        retainedBytes_.fetch_add(estBytes, std::memory_order_relaxed);

        if (count >= kHardThreshold)
        {
            hardBreachCount_.fetch_add(1, std::memory_order_relaxed);
        }
        else if (count >= kSoftThreshold && !softWarningFired_)
        {
            softWarningFired_ = true;
            // Debug builds: break to inspect retained state growth.
            jassertfalse;
        }
    }

    uint64_t retainedCount() const noexcept { return retainedCount_.load(std::memory_order_relaxed); }

    uint64_t retainedEstimatedBytes() const noexcept { return retainedBytes_.load(std::memory_order_relaxed); }

    uint64_t hardBreachCount() const noexcept { return hardBreachCount_.load(std::memory_order_relaxed); }

    // Thresholds - tuneable per release cycle.
    static constexpr size_t kSoftThreshold = 500;
    static constexpr size_t kHardThreshold = 2000;

private:
    ValueTreeRetainer() = default;
    inline static bool testingMode_ = false;

    static uint64_t estimateBytes(const juce::ValueTree& state) noexcept
    {
        uint64_t bytes = 128; // SharedObject overhead
        bytes += static_cast<uint64_t>(state.getNumProperties()) * 64;
        for (int i = 0; i < state.getNumChildren(); ++i)
            bytes += estimateBytes(state.getChild(i));
        return bytes;
    }

    std::vector<juce::ValueTree> retained_;
    std::atomic<uint64_t> retainedCount_ {0};
    std::atomic<uint64_t> retainedBytes_ {0};
    std::atomic<uint64_t> hardBreachCount_ {0};
    bool softWarningFired_ = false;
};

} // namespace bws::adapters
