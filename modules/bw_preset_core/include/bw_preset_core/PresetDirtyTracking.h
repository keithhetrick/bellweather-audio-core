// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <atomic>
#include <cstdint>

namespace bws::preset
{

class AtomicPresetDirtyTracker
{
public:
    [[nodiscard]] bool isEligible() const { return eligible_.load(std::memory_order_acquire); }

    [[nodiscard]] bool isSuppressed() const { return suppressionDepth_.load(std::memory_order_acquire) > 0; }

    [[nodiscard]] bool hasPendingDirtyChange() const
    {
        return pendingDirty_.load(std::memory_order_acquire) && isEligible() &&
               pendingGeneration_.load(std::memory_order_acquire) == generation_.load(std::memory_order_acquire);
    }

    void reset(bool eligible)
    {
        pendingDirty_.store(false, std::memory_order_release);
        pendingGeneration_.store(0, std::memory_order_release);
        eligible_.store(eligible, std::memory_order_release);
        advanceGeneration();
    }

    void beginSuppression() { suppressionDepth_.fetch_add(1, std::memory_order_acq_rel); }

    void endSuppression()
    {
        pendingDirty_.store(false, std::memory_order_release);
        pendingGeneration_.store(0, std::memory_order_release);
        advanceGeneration();
        suppressionDepth_.fetch_sub(1, std::memory_order_acq_rel);
    }

    void markDirtyFromRealtime()
    {
        if (isSuppressed() || !isEligible() || pendingDirty_.load(std::memory_order_acquire))
        {
            return;
        }

        pendingGeneration_.store(generation_.load(std::memory_order_acquire), std::memory_order_release);
        pendingDirty_.store(true, std::memory_order_release);
    }

    [[nodiscard]] bool consumePendingDirtyChange()
    {
        if (isSuppressed())
            return false;

        if (!hasPendingDirtyChange())
        {
            if (pendingDirty_.load(std::memory_order_acquire))
                pendingDirty_.store(false, std::memory_order_release);
            return false;
        }

        pendingDirty_.store(false, std::memory_order_release);
        return true;
    }

private:
    void advanceGeneration() { generation_.fetch_add(1, std::memory_order_acq_rel); }

    std::atomic<bool> pendingDirty_ {false};
    std::atomic<bool> eligible_ {false};
    std::atomic<int> suppressionDepth_ {0};
    std::atomic<uint64_t> generation_ {0};
    std::atomic<uint64_t> pendingGeneration_ {0};
};

} // namespace bws::preset
