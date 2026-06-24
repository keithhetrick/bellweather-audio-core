// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <atomic>
#include <algorithm>
#include <bw_dsp_core/concepts/DspConcepts.h>
#include <bw_audio_types/BufferView.h>

namespace bws
{
namespace dsp
{

/**
 * @brief Swaps left and right audio channels
 *
 * Exchanges the left and right channels of a stereo buffer. Useful for
 * correcting channel inversions or creative stereo manipulation.
 *
 * ## Real-Time Safety
 * - prepare() and reset() are NOT real-time safe
 * - setEnabled() IS real-time safe (atomic write)
 * - process() IS real-time safe (no allocations, no locks, no syscalls)
 *
 * ## Usage Example
 * @code
 * bws::dsp::ChannelSwap swapper;
 * swapper.prepare(44100.0, 512);
 *
 * // In processBlock():
 * bool shouldSwap = getParameter();
 * swapper.setEnabled(shouldSwap);
 * swapper.process(buffer);
 * @endcode
 *
 * @see ChannelSolo, MonoSum
 */
class ChannelSwap
{
public:
    /**
     * @brief Construct a new ChannelSwap with swapping disabled
     */
    ChannelSwap() = default;

    /**
     * @brief Destructor
     */
    ~ChannelSwap() = default;

    /**
     * @brief Non-copyable
     */
    ChannelSwap(const ChannelSwap&) = delete;
    ChannelSwap& operator=(const ChannelSwap&) = delete;

    /**
     * @brief Prepares the processor for processing
     *
     * @param sampleRate Sample rate in Hz (unused)
     * @param maxBlockSize Maximum block size (unused)
     *
     * @note Not real-time safe. Call before audio processing starts.
     */
    void prepare(double sampleRate, int maxBlockSize)
    {
        (void)sampleRate;
        (void)maxBlockSize;
        reset();
    }

    /**
     * @brief Resets processor state
     *
     * Sets enabled state to false.
     *
     * @note Not real-time safe. Call when audio processing stops.
     */
    void reset() { enabled_.store(false, std::memory_order_relaxed); }

    /**
     * @brief Enable or disable channel swapping
     *
     * @param enable True to swap channels, false for normal routing
     *
     * @note Real-time safe. Call this before process() in your processBlock().
     */
    void setEnabled(bool enable) { enabled_.store(enable, std::memory_order_relaxed); }

    /**
     * @brief Get current enabled state
     *
     * @return true if channel swapping is enabled
     */
    bool isEnabled() const { return enabled_.load(std::memory_order_relaxed); }

    /**
     * @brief Process audio buffer
     *
     * Swaps left and right channels if enabled. Mono buffers are unaffected.
     *
     * @param buffer Audio buffer to process (modified in-place)
     *
     * @note Real-time safe. No allocations, no locks.
     */
    void process(bws::domain::BufferView buffer)
    {
        const int numChannels = buffer.numChannels;
        const int numSamples = buffer.numSamples;

        if (numChannels < 2 || numSamples == 0)
            return;

        const bool shouldSwap = enabled_.load(std::memory_order_relaxed);

        if (!shouldSwap)
            return;

        float* __restrict L = buffer.channel(0);
        float* __restrict R = buffer.channel(1);

        for (int i = 0; i < numSamples; ++i)
        {
            std::swap(L[i], R[i]);
        }
    }

private:
    std::atomic<bool> enabled_ {false}; ///< Enable channel swapping
};

} // namespace dsp
} // namespace bws

static_assert(bws::dsp::BufferDSP<bws::dsp::ChannelSwap>);
