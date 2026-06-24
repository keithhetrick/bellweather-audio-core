// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <bw_audio_types/BwsLinearSmoothedValue.h>
#include <atomic>
#include <bw_dsp_core/concepts/DspConcepts.h>
#include <bw_audio_types/BufferView.h>

namespace bws
{
namespace dsp
{

/**
 * @brief Stereo balance/pan control with smoothing
 *
 * Applies linear pan law to adjust stereo balance. Features per-sample
 * smoothing for zipper-free parameter changes.
 *
 * ## Real-Time Safety
 * - prepare() and reset() are NOT real-time safe
 * - setBalance() IS real-time safe (atomic write)
 * - process() IS real-time safe (no allocations, no locks, no syscalls)
 *
 * ## Usage Example
 * @code
 * bws::dsp::StereoBalancer balancer;
 * balancer.prepare(44100.0, 512);
 *
 * // In processBlock():
 * float pan = getParameter(); // -1.0 (left) to +1.0 (right)
 * balancer.setBalance(pan);
 * balancer.process(buffer);
 * @endcode
 *
 * @see StereoWidth, PhaseInverter
 */
class StereoBalancer
{
public:
    /**
     * @brief Construct a new StereoBalancer centered (0.0)
     */
    StereoBalancer() = default;

    /**
     * @brief Destructor
     */
    ~StereoBalancer() = default;

    /**
     * @brief Non-copyable
     */
    StereoBalancer(const StereoBalancer&) = delete;
    StereoBalancer& operator=(const StereoBalancer&) = delete;

    /**
     * @brief Prepares the processor for processing
     *
     * @param sampleRate Sample rate in Hz
     * @param maxBlockSize Maximum block size (unused)
     *
     * @note Not real-time safe. Call before audio processing starts.
     */
    void prepare(double sampleRate, int maxBlockSize)
    {
        (void)maxBlockSize;

        // 50ms smoothing time for smooth panning
        smoothedBalance_.reset(sampleRate, 0.05);
        smoothedBalance_.setCurrentAndTargetValue(0.0f);

        reset();
    }

    /**
     * @brief Resets processor state
     *
     * Sets balance to center (0.0).
     *
     * @note Not real-time safe. Call when audio processing stops.
     */
    void reset()
    {
        balance_.store(0.0f, std::memory_order_relaxed);
        smoothedBalance_.setCurrentAndTargetValue(0.0f);
    }

    /**
     * @brief Set balance/pan position
     *
     * @param balance Pan value: -1.0 (full left) to +1.0 (full right), 0.0 (center)
     *
     * @note Real-time safe. Call this before process() in your processBlock().
     */
    void setBalance(float balance) { balance_.store(balance, std::memory_order_relaxed); }

    /**
     * @brief Get current balance setting
     *
     * @return Current balance value (-1.0 to +1.0)
     */
    float getBalance() const { return balance_.load(std::memory_order_relaxed); }

    /**
     * @brief Process audio buffer
     *
     * Applies linear pan law with per-sample smoothing:
     * - Pan -1.0: Left = 1.0, Right = 0.0
     * - Pan  0.0: Left = 1.0, Right = 1.0
     * - Pan +1.0: Left = 0.0, Right = 1.0
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

        const float balanceValue = balance_.load(std::memory_order_relaxed);
        smoothedBalance_.setTargetValue(balanceValue);

        float* __restrict leftData = buffer.channel(0);
        float* __restrict rightData = buffer.channel(1);

        for (int i = 0; i < numSamples; ++i)
        {
            const float panValue = smoothedBalance_.getNextValue();

            // Linear pan law
            const float leftGain = (panValue <= 0.0f) ? 1.0f : (1.0f - panValue);
            const float rightGain = (panValue >= 0.0f) ? 1.0f : (1.0f + panValue);

            leftData[i] *= leftGain;
            rightData[i] *= rightGain;
        }
    }

private:
    std::atomic<float> balance_ {0.0f};                          ///< Balance value (-1.0 to +1.0)
    bws::domain::BwsLinearSmoothedValue smoothedBalance_ {0.0f}; ///< Smoothed balance for zipper-free changes
};

} // namespace dsp
} // namespace bws

static_assert(bws::dsp::BufferDSP<bws::dsp::StereoBalancer>);
