// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <atomic>
#include <cmath>
#include <bw_dsp_core/concepts/DspConcepts.h>
#include <bw_audio_types/BufferView.h>

namespace bws
{
namespace dsp
{

/**
 * @brief Adjusts stereo width using Mid-Side processing
 *
 * Encodes L/R to Mid/Side, scales the Side component by a width factor,
 * then decodes back to L/R. Width of 1.0 is neutral, <1.0 narrows,
 * >1.0 widens the stereo image.
 *
 * ## Real-Time Safety
 * - prepare() and reset() are NOT real-time safe
 * - setWidth() IS real-time safe (atomic write)
 * - process() IS real-time safe (no allocations, no locks, no syscalls)
 *
 * ## Usage Example
 * @code
 * bws::dsp::StereoWidth widthProcessor;
 * widthProcessor.prepare(44100.0, 512);
 *
 * // In processBlock():
 * float width = getParameter(); // 0.0 (mono) to 2.0 (super wide)
 * widthProcessor.setWidth(width);
 * widthProcessor.process(buffer);
 * @endcode
 *
 * @see StereoBalancer, MonoSum
 */
class StereoWidth
{
public:
    /**
     * @brief Construct a new StereoWidth with neutral width (1.0)
     */
    StereoWidth() = default;

    /**
     * @brief Destructor
     */
    ~StereoWidth() = default;

    /**
     * @brief Non-copyable
     */
    StereoWidth(const StereoWidth&) = delete;
    StereoWidth& operator=(const StereoWidth&) = delete;

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
     * Sets width to 1.0 (neutral).
     *
     * @note Not real-time safe. Call when audio processing stops.
     */
    void reset() { width_.store(1.0f, std::memory_order_relaxed); }

    /**
     * @brief Set stereo width factor
     *
     * @param width Width multiplier (0.0 = mono, 1.0 = neutral, 2.0 = double-wide)
     *
     * @note Real-time safe. Call this before process() in your processBlock().
     */
    void setWidth(float width) { width_.store(width, std::memory_order_relaxed); }

    /**
     * @brief Get current width
     *
     * @return Current width factor
     */
    float getWidth() const { return width_.load(std::memory_order_relaxed); }

    /**
     * @brief Process audio buffer
     *
     * Applies Mid-Side width processing. If width is ~1.0 (within 0.001),
     * processing is bypassed for efficiency.
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

        const float widthValue = width_.load(std::memory_order_relaxed);

        // Skip processing if width is effectively neutral
        if (std::abs(widthValue - 1.0f) < 0.001f)
            return;

        float* __restrict L = buffer.channel(0);
        float* __restrict R = buffer.channel(1);

        for (int i = 0; i < numSamples; ++i)
        {
            // Encode to Mid-Side
            const float mid = (L[i] + R[i]) * 0.5f;
            float side = (L[i] - R[i]) * 0.5f;

            // Scale side by width
            side *= widthValue;

            // Decode back to L/R
            L[i] = mid + side;
            R[i] = mid - side;
        }
    }

private:
    std::atomic<float> width_ {1.0f}; ///< Width factor (0.0 to 2.0+)
};

} // namespace dsp
} // namespace bws

static_assert(bws::dsp::BufferDSP<bws::dsp::StereoWidth>);
