// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <atomic>
#include <cstring>
#include <bw_dsp_core/concepts/DspConcepts.h>
#include <bw_audio_types/BufferView.h>

namespace bws
{
namespace dsp
{

/**
 * @brief Mutes left or right channel for solo monitoring
 *
 * Allows independent muting of stereo channels. Useful for checking channel
 * balance, phase issues, or creative stereo manipulation.
 *
 * ## Real-Time Safety
 * - prepare() and reset() are NOT real-time safe
 * - setSoloLeft() and setSoloRight() ARE real-time safe (atomic writes)
 * - process() IS real-time safe (no allocations, no locks, no syscalls)
 *
 * ## Usage Example
 * @code
 * bws::dsp::ChannelSolo solo;
 * solo.prepare(44100.0, 512);
 *
 * // In processBlock():
 * solo.setSoloLeft(soloLButton);
 * solo.setSoloRight(soloRButton);
 * solo.process(buffer);
 * @endcode
 *
 * @see ChannelSwap, MonoSum
 */
class ChannelSolo
{
public:
    /**
     * @brief Construct a new ChannelSolo with both channels playing
     */
    ChannelSolo() = default;

    /**
     * @brief Destructor
     */
    ~ChannelSolo() = default;

    /**
     * @brief Non-copyable
     */
    ChannelSolo(const ChannelSolo&) = delete;
    ChannelSolo& operator=(const ChannelSolo&) = delete;

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
     * Sets both solo states to false.
     *
     * @note Not real-time safe. Call when audio processing stops.
     */
    void reset()
    {
        soloLeft_.store(false, std::memory_order_relaxed);
        soloRight_.store(false, std::memory_order_relaxed);
    }

    /**
     * @brief Enable left channel solo (mutes right)
     *
     * @param solo True to solo left channel
     *
     * @note Real-time safe. Call this before process() in your processBlock().
     */
    void setSoloLeft(bool solo) { soloLeft_.store(solo, std::memory_order_relaxed); }

    /**
     * @brief Enable right channel solo (mutes left)
     *
     * @param solo True to solo right channel
     *
     * @note Real-time safe. Call this before process() in your processBlock().
     */
    void setSoloRight(bool solo) { soloRight_.store(solo, std::memory_order_relaxed); }

    /**
     * @brief Get left solo state
     *
     * @return true if left channel is soloed
     */
    bool isLeftSoloed() const { return soloLeft_.load(std::memory_order_relaxed); }

    /**
     * @brief Get right solo state
     *
     * @return true if right channel is soloed
     */
    bool isRightSoloed() const { return soloRight_.load(std::memory_order_relaxed); }

    /**
     * @brief Process audio buffer
     *
     * Behavior:
     * - Solo left only: Right channel muted
     * - Solo right only: Left channel muted
     * - Both soloed: Both channels pass through
     * - Neither soloed: Both channels pass through
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

        const bool soloL = soloLeft_.load(std::memory_order_relaxed);
        const bool soloR = soloRight_.load(std::memory_order_relaxed);

        if (soloL && !soloR)
        {
            // Solo left: mute right channel
            std::memset(buffer.channel(1), 0, sizeof(float) * static_cast<size_t>(numSamples));
        }
        else if (soloR && !soloL)
        {
            // Solo right: mute left channel
            std::memset(buffer.channel(0), 0, sizeof(float) * static_cast<size_t>(numSamples));
        }
        // If both solo'd or neither, play both channels
    }

private:
    std::atomic<bool> soloLeft_ {false};  ///< Solo left channel flag
    std::atomic<bool> soloRight_ {false}; ///< Solo right channel flag
};

} // namespace dsp
} // namespace bws

static_assert(bws::dsp::BufferDSP<bws::dsp::ChannelSolo>);
