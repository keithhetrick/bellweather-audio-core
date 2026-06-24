// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <atomic>
#include <vector>
#include <algorithm>
#include <cstdint>

namespace bws::rt
{

//==============================================================================
// LockFreeRingBuffer - SPSC lock-free multi-channel audio ring buffer.
//
// Thread model:
//   - Single producer (audio thread) calls push()
//   - Single consumer (worker thread) calls readChannel() / consumeSamples()
//   - Both threads may call availableToRead/WriteSamples(), getWriteCounter()
//
// Memory ordering:
//   - push() uses relaxed loads + release store on writeCounter_
//   - consumeSamples() uses relaxed load + release store on readCounter_
//   - Query methods use acquire loads
//
// Storage: flat interleaved-by-channel vector [ch0_0..ch0_N, ch1_0..ch1_N]
// Counters: unbounded uint64_t (wrap after ~292 billion years at 48kHz)
//==============================================================================

class LockFreeRingBuffer
{
public:
    //--------------------------------------------------------------------------
    // Setup (NOT RT-safe - allocates)
    //--------------------------------------------------------------------------

    void prepare(int numChannels, int capacitySamples)
    {
        numChannels_ = std::max(1, std::min(numChannels, 2));
        capacity_ = std::max(1, capacitySamples);
        buffer_.assign(static_cast<size_t>(numChannels_) * static_cast<size_t>(capacity_), 0.0f);
        writeCounter_.store(0, std::memory_order_release);
        readCounter_.store(0, std::memory_order_release);
    }

    //--------------------------------------------------------------------------
    // Query (RT-safe)
    //--------------------------------------------------------------------------

    int availableToWriteSamples() const noexcept
    {
        const auto w = writeCounter_.load(std::memory_order_acquire);
        const auto r = readCounter_.load(std::memory_order_acquire);
        const uint64_t used = w - r;
        if (used >= static_cast<uint64_t>(capacity_))
            return 0;
        return capacity_ - static_cast<int>(used);
    }

    int availableToReadSamples() const noexcept
    {
        const auto w = writeCounter_.load(std::memory_order_acquire);
        const auto r = readCounter_.load(std::memory_order_acquire);
        const uint64_t used = w - r;
        return static_cast<int>(std::min<uint64_t>(used, static_cast<uint64_t>(capacity_)));
    }

    int getCapacity() const noexcept { return capacity_; }
    int getNumChannels() const noexcept { return numChannels_; }
    uint64_t getWriteCounter() const noexcept { return writeCounter_.load(std::memory_order_acquire); }

    //--------------------------------------------------------------------------
    // Producer (audio thread, RT-safe)
    //--------------------------------------------------------------------------

    bool push(const float* const* channelPtrs, int numChannels, int numSamples) noexcept
    {
        const int channels = std::min(numChannels_, numChannels);
        const int cap = capacity_;
        const auto writeCount = writeCounter_.load(std::memory_order_relaxed);
        const auto readCount = readCounter_.load(std::memory_order_relaxed);
        const uint64_t used = writeCount - readCount;
        if (used + static_cast<uint64_t>(numSamples) > static_cast<uint64_t>(capacity_))
            return false;

        for (int i = 0; i < numSamples; ++i)
        {
            const int idx = static_cast<int>((writeCount + static_cast<uint64_t>(i)) % static_cast<uint64_t>(cap));
            for (int ch = 0; ch < channels; ++ch)
                buffer_[static_cast<size_t>(ch) * static_cast<size_t>(cap) + static_cast<size_t>(idx)] =
                    channelPtrs[ch][i];
        }
        writeCounter_.store(writeCount + static_cast<uint64_t>(numSamples), std::memory_order_release);
        return true;
    }

    //--------------------------------------------------------------------------
    // Consumer (worker thread)
    //--------------------------------------------------------------------------

    void consumeSamples(int numSamples) noexcept
    {
        const auto r = readCounter_.load(std::memory_order_relaxed);
        readCounter_.store(r + static_cast<uint64_t>(std::max(0, numSamples)), std::memory_order_release);
    }

    /// Read the latest numSamples from the readable window for a single channel.
    /// Reads from readCounter_ position forward. Does NOT advance the read pointer.
    void readChannel(int channel, float* dst, int numSamples) const noexcept
    {
        if (buffer_.empty() || numSamples <= 0 || channel < 0 || channel >= numChannels_)
            return;

        const uint64_t writeCount = writeCounter_.load(std::memory_order_acquire);
        const uint64_t readCount = readCounter_.load(std::memory_order_acquire);
        const uint64_t available = writeCount > readCount ? (writeCount - readCount) : 0ULL;
        if (available == 0)
            return;

        const int cap = capacity_;
        const int toCopy =
            std::min<int>(numSamples, static_cast<int>(std::min<uint64_t>(available, static_cast<uint64_t>(cap))));
        const size_t chOffset = static_cast<size_t>(channel) * static_cast<size_t>(cap);
        const uint64_t start = readCount;
        for (int i = 0; i < toCopy; ++i)
        {
            const uint64_t idx = (start + static_cast<uint64_t>(i)) % static_cast<uint64_t>(cap);
            dst[i] = buffer_[chOffset + static_cast<size_t>(idx)];
        }
    }

private:
    int numChannels_ {0};
    int capacity_ {0};
    std::vector<float> buffer_;
    std::atomic<uint64_t> writeCounter_ {0};
    std::atomic<uint64_t> readCounter_ {0};
};

} // namespace bws::rt
