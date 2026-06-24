// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <atomic>
#include <bw_dsp_core/concepts/DspConcepts.h>
#include <bw_audio_types/BufferView.h>

namespace bws
{
namespace dsp
{

/**
 * @brief Sums stereo channels to mono
 *
 * Converts stereo to mono by averaging the channels ((L+R)/2) and duplicating
 * the result to both. This is level-preserving for correlated content (no
 * +6 dB summing gain / clipping risk) - the correct law for mono-compatibility
 * checking. Useful for mono compatibility checking or creative mono summing.
 *
 * ## Real-Time Safety
 * - prepare() and reset() are NOT real-time safe
 * - setEnabled() IS real-time safe (atomic write)
 * - process() IS real-time safe (no allocations, no locks, no syscalls)
 *
 * ## Usage Example
 * @code
 * bws::dsp::MonoSum monoProcessor;
 * monoProcessor.prepare(44100.0, 512);
 *
 * // In processBlock():
 * bool monoEnabled = getParameter();
 * monoProcessor.setEnabled(monoEnabled);
 * monoProcessor.process(buffer);
 * @endcode
 *
 * @see StereoWidth, ChannelSwap
 */
class MonoSum
{
public:
    /**
     * @brief Construct a new MonoSum with processing disabled
     */
    MonoSum() = default;

    /**
     * @brief Destructor
     */
    ~MonoSum() = default;

    /**
     * @brief Non-copyable
     */
    MonoSum(const MonoSum&) = delete;
    MonoSum& operator=(const MonoSum&) = delete;

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
     * @brief Enable or disable mono summing
     *
     * @param enable True to sum to mono, false for stereo passthrough
     *
     * @note Real-time safe. Call this before process() in your processBlock().
     */
    void setEnabled(bool enable) { enabled_.store(enable, std::memory_order_relaxed); }

    /**
     * @brief Get current enabled state
     *
     * @return true if mono summing is enabled
     */
    bool isEnabled() const { return enabled_.load(std::memory_order_relaxed); }

    /**
     * @brief Process audio buffer
     *
     * Sums stereo to mono: (L+R)/2, then duplicates to both channels.
     * Mono buffers are unaffected.
     *
     * @param buffer Audio buffer to process
     *
     * @note Real-time safe. No allocations, no locks.
     */
    void process(bws::domain::BufferView buffer)
    {
        const int numChannels = buffer.numChannels;
        const int numSamples = buffer.numSamples;

        if (numChannels < 2 || numSamples == 0)
            return;

        const bool shouldSum = enabled_.load(std::memory_order_relaxed);

        if (!shouldSum)
            return;

        float* __restrict L = buffer.channel(0);
        float* __restrict R = buffer.channel(1);

        for (int i = 0; i < numSamples; ++i)
        {
            const float mono = (L[i] + R[i]) * 0.5f;
            L[i] = mono;
            R[i] = mono;
        }
    }

private:
    std::atomic<bool> enabled_ {false}; ///< Enable mono summing
};

} // namespace dsp
} // namespace bws

static_assert(bws::dsp::BufferDSP<bws::dsp::MonoSum>);
