// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>

namespace bws
{
namespace dsp
{

/**
 * @brief Stereo peak level meter with ballistics
 *
 * Measures peak levels independently for left and right channels with
 * configurable release time. Features instant attack and smooth release
 * for natural-looking meters.
 *
 * ## Real-Time Safety
 * - prepare() and reset() are NOT real-time safe
 * - processSamples() IS real-time safe (no allocations, no locks, no syscalls)
 * - getLeftPeakDb()/getRightPeakDb() ARE real-time safe (atomic reads)
 *
 * ## Usage Example
 * @code
 * bws::dsp::StereoLevelMeter meter;
 * meter.prepare(44100.0);
 *
 * // In processBlock():
 * meter.processSamples(buffer.getReadPointer(0),
 *                      buffer.getReadPointer(1),
 *                      buffer.getNumSamples());
 *
 * // In UI thread:
 * float leftDb = meter.getLeftPeakDb();
 * float rightDb = meter.getRightPeakDb();
 * @endcode
 *
 * @see StereoBalancer, StereoWidth
 */
class StereoLevelMeter
{
public:
    /**
     * @brief Construct a new StereoLevelMeter with silence (-100 dB)
     */
    StereoLevelMeter() = default;

    /**
     * @brief Destructor
     */
    ~StereoLevelMeter() = default;

    /**
     * @brief Non-copyable
     */
    StereoLevelMeter(const StereoLevelMeter&) = delete;
    StereoLevelMeter& operator=(const StereoLevelMeter&) = delete;

    /**
     * @brief Prepares the meter for processing
     *
     * @param sampleRate Sample rate in Hz
     *
     * @note Not real-time safe. Call before audio processing starts.
     */
    void prepare(double sampleRate)
    {
        sampleRate_ = sampleRate;

        // Pre-calculate release coefficient
        const float tau = kReleaseTimeMs * 0.001f;
        releaseCoeffPerSample_ = std::exp(-1.0f / (tau * static_cast<float>(sampleRate)));

        reset();
    }

    /**
     * @brief Resets meter state to silence
     *
     * @note Not real-time safe. Call when audio processing stops.
     */
    void reset()
    {
        leftPeakDb_.store(-100.0f, std::memory_order_relaxed);
        rightPeakDb_.store(-100.0f, std::memory_order_relaxed);
        smoothedLeftPeak_ = -100.0f;
        smoothedRightPeak_ = -100.0f;
    }

    /**
     * @brief Process audio samples
     *
     * @param left Left channel samples
     * @param right Right channel samples (can be nullptr for mono)
     * @param numSamples Number of samples to process
     *
     * @note Real-time safe. Call from audio thread.
     */
    void processSamples(const float* left, const float* right, int numSamples)
    {
        if (numSamples == 0)
            return;

        float blockPeakL = 0.0f;
        float blockPeakR = 0.0f;

        // Find peak in block
        for (int i = 0; i < numSamples; ++i)
        {
            const float sampleL = std::abs(left[i]);
            const float sampleR = right ? std::abs(right[i]) : sampleL;

            if (sampleL > blockPeakL)
                blockPeakL = sampleL;
            if (sampleR > blockPeakR)
                blockPeakR = sampleR;
        }

        // Convert to dB
        const float blockPeakDbL = (blockPeakL > 0.0f) ? 20.0f * std::log10(blockPeakL) : -100.0f;
        const float blockPeakDbR = (blockPeakR > 0.0f) ? 20.0f * std::log10(blockPeakR) : -100.0f;

        // Apply ballistics
        const float decayForBlock = std::pow(releaseCoeffPerSample_, static_cast<float>(numSamples));

        // Left channel
        if (blockPeakDbL > smoothedLeftPeak_)
        {
            smoothedLeftPeak_ = blockPeakDbL; // Instant attack
        }
        else
        {
            smoothedLeftPeak_ = blockPeakDbL + (smoothedLeftPeak_ - blockPeakDbL) * decayForBlock;
        }
        smoothedLeftPeak_ = std::max(smoothedLeftPeak_, -100.0f);

        // Right channel
        if (blockPeakDbR > smoothedRightPeak_)
        {
            smoothedRightPeak_ = blockPeakDbR; // Instant attack
        }
        else
        {
            smoothedRightPeak_ = blockPeakDbR + (smoothedRightPeak_ - blockPeakDbR) * decayForBlock;
        }
        smoothedRightPeak_ = std::max(smoothedRightPeak_, -100.0f);

        // Store to atomics
        leftPeakDb_.store(smoothedLeftPeak_, std::memory_order_relaxed);
        rightPeakDb_.store(smoothedRightPeak_, std::memory_order_relaxed);
    }

    /**
     * @brief Get left channel peak level in dB
     *
     * @return Peak level in dB (-100 to 0+)
     *
     * @note Thread-safe. Can be called from UI thread.
     */
    float getLeftPeakDb() const { return leftPeakDb_.load(std::memory_order_relaxed); }

    /**
     * @brief Get right channel peak level in dB
     *
     * @return Peak level in dB (-100 to 0+)
     *
     * @note Thread-safe. Can be called from UI thread.
     */
    float getRightPeakDb() const { return rightPeakDb_.load(std::memory_order_relaxed); }

    /**
     * @brief Get combined (max of L/R) peak level in dB
     *
     * @return Peak level in dB (-100 to 0+)
     *
     * @note Thread-safe. Can be called from UI thread.
     */
    float getPeakLevelDb() const { return std::max(getLeftPeakDb(), getRightPeakDb()); }

private:
    static constexpr float kReleaseTimeMs = 300.0f; ///< 300ms decay time

    double sampleRate_ = 44100.0;           ///< Current sample rate
    float releaseCoeffPerSample_ = 0.9999f; ///< Pre-calculated release coefficient

    std::atomic<float> leftPeakDb_ {-100.0f};  ///< Left channel peak level
    std::atomic<float> rightPeakDb_ {-100.0f}; ///< Right channel peak level

    float smoothedLeftPeak_ = -100.0f;  ///< Internal smoothed left peak
    float smoothedRightPeak_ = -100.0f; ///< Internal smoothed right peak
};

} // namespace dsp
} // namespace bws
