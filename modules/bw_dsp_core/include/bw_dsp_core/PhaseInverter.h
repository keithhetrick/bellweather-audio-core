// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// bws::dsp::hot: per-sample hot path. Keep loops simple and compiler-visible;
//

#include <bw_audio_types/BufferView.h>
#include <atomic>
#include <bw_dsp_core/concepts/DspConcepts.h>

namespace bws
{
namespace dsp
{

/**
 * @brief Inverts audio signal polarity per channel
 *
 * Applies phase inversion (180° polarity flip) independently to left and right
 * channels. Useful for correcting phase issues, stereo manipulation, or creative
 * effects.
 *
 * ## Real-Time Safety
 * - prepare() and reset() are NOT real-time safe
 * - setInvertLeft() and setInvertRight() ARE real-time safe (atomic writes)
 * - process() IS real-time safe (no allocations, no locks, no syscalls)
 *
 * ## Usage Example
 * @code
 * bws::dsp::PhaseInverter inverter;
 * inverter.prepare(44100.0, 512);
 *
 * // In processBlock():
 * inverter.setInvertLeft(shouldInvertLeft);
 * inverter.setInvertRight(shouldInvertRight);
 * inverter.process(buffer);
 * @endcode
 *
 * @see StereoBalancer, StereoWidth
 */
class PhaseInverter
{
public:
    /**
     * @brief Construct a new nverter with both channels non-inverted
     */
    PhaseInverter() = default;

    /**
     * @brief Prepare the processor for processing
     *
     * @param sampleRate The sample rate in Hz
     * @param maxBlockSize The maximum number of samples per block
     *
     * @note Not real-time safe. Call before audio processing starts.
     */
    void prepare(double sampleRate, int maxBlockSize);

    /**
     * @brief Reset the processor to initial state
     *
     * Sets both channels to non-inverted state.
     *
     * @note Not real-time safe. Call when audio processing stops.
     */
    void reset();

    /**
     * @brief Set whether to invert the left channel
     *
     * @param invert True to invert, false for normal polarity
     *
     * @note Real-time safe. Call this before process() in your processBlock().
     */
    void setInvertLeft(bool invert);

    /**
     * @brief Set whether to invert the right channel
     *
     * @param invert True to invert, false for normal polarity
     *
     * @note Real-time safe. Call this before process() in your processBlock().
     */
    void setInvertRight(bool invert);

    /**
     * @brief Get current left channel inversion state
     *
     * @return true if left channel is inverted
     */
    bool isLeftInverted() const { return invertL_.load(std::memory_order_relaxed); }

    /**
     * @brief Get current right channel inversion state
     *
     * @return true if right channel is inverted
     */
    bool isRightInverted() const { return invertR_.load(std::memory_order_relaxed); }

    /**
     * @brief Process audio buffer with phase inversion
     *
     * Applies phase inversion to channels based on current settings. Works with
     * mono (1 channel) or stereo (2+ channels) buffers. For mono buffers, only
     * the left channel setting is used.
     *
     * @param buffer Audio buffer to process (modified in place)
     *
     * @note Real-time safe. No allocations, no locks, no syscalls.
     */
    void process(bws::domain::BufferView buffer);

private:
    std::atomic<bool> invertL_ {false}; ///< Left channel inversion flag
    std::atomic<bool> invertR_ {false}; ///< Right channel inversion flag
};

//==============================================================================
// Implementation
//==============================================================================

inline void PhaseInverter::prepare(double /*sampleRate*/, int /*maxBlockSize*/)
{
    // No preparation needed for this simple processor
    // Parameters unused but kept for interface consistency
}

inline void PhaseInverter::reset()
{
    invertL_.store(false, std::memory_order_relaxed);
    invertR_.store(false, std::memory_order_relaxed);
}

inline void PhaseInverter::setInvertLeft(bool invert)
{
    invertL_.store(invert, std::memory_order_relaxed);
}

inline void PhaseInverter::setInvertRight(bool invert)
{
    invertR_.store(invert, std::memory_order_relaxed);
}

inline void PhaseInverter::process(bws::domain::BufferView buffer)
{
    const int numChannels = buffer.numChannels;
    const int numSamples = buffer.numSamples;

    if (numSamples == 0 || numChannels == 0)
        return;

    // Read atomic flags (RT-safe)
    const bool shouldInvertL = invertL_.load(std::memory_order_relaxed);
    const bool shouldInvertR = invertR_.load(std::memory_order_relaxed);

    // Invert left channel if requested
    if (shouldInvertL && numChannels > 0)
    {
        float* __restrict data = buffer.channel(0);
        for (int i = 0; i < numSamples; ++i)
            data[i] = -data[i];
    }

    // Invert right channel if requested (only if stereo)
    if (shouldInvertR && numChannels > 1)
    {
        float* __restrict data = buffer.channel(1);
        for (int i = 0; i < numSamples; ++i)
            data[i] = -data[i];
    }
}

} // namespace dsp
} // namespace bws

static_assert(bws::dsp::BufferDSP<bws::dsp::PhaseInverter>);
