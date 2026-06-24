// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once

/**
 * ============================================================================
 * MID/SIDE PROCESSOR MODULE
 * ============================================================================
 *
 * Mid/Side stereo encoding and decoding for independent center/width processing.
 *
 * USE CASES:
 * - Stereo width control: Boost Side for wider image, cut for narrower
 * - Center vocal isolation: Process Mid channel independently
 * - Mastering EQ: Different EQ curves for center vs stereo content
 * - Dynamics: Compress Mid differently than Side
 * - Bass mono-ization: Apply lowpass to Side to mono the bass
 *
 * ============================================================================
 *                              ARCHITECTURE
 * ============================================================================
 *
 *                    MID/SIDE ENCODING CONCEPT
 *
 *
 *     Standard stereo (L/R) represents:
 *       - Left speaker content
 *       - Right speaker content
 *
 *     Mid/Side (M/S) represents:
 *       - Mid = Center content (what's the same in L and R)
 *       - Side = Difference content (what's different between L and R)
 *
 *
 *                    ENCODING: L/R to M/S
 *
 *
 *        Left ─────┬───────────────────────┐
 *                  │                       │
 *                  ▼                       ▼
 *             ┌─────────┐             ┌─────────┐
 *             │    +    │             │    -    │
 *             │  (sum)  │             │ (diff)  │
 *             └────┬────┘             └────┬────┘
 *                  │                       │
 *                  ▼                       ▼
 *             ┌─────────┐             ┌─────────┐
 *             │  × 0.5  │             │  × 0.5  │
 *             └────┬────┘             └────┬────┘
 *                  │                       │
 *        Right ────┘                       │
 *                  │                       │
 *                  ▼                       ▼
 *                 Mid                    Side
 *            (L+R)/2                   (L-R)/2
 *
 *
 *                    DECODING: M/S to L/R
 *
 *
 *         Mid ──────┬───────────────────────┐
 *                   │                       │
 *                   ▼                       ▼
 *              ┌─────────┐             ┌─────────┐
 *              │    +    │             │    -    │
 *              │  (sum)  │             │ (diff)  │
 *              └────┬────┘             └────┬────┘
 *                   │                       │
 *                   ▲                       ▲
 *        Side ──────┴───────────────────────┘
 *                   │                       │
 *                   ▼                       ▼
 *                 Left                   Right
 *                M + S                   M - S
 *
 *
 *                    STEREO WIDTH VISUALIZATION
 *
 *
 *     Mono signal (no Side):          Wide stereo (boosted Side):
 *
 *           L ════ R                        L ─────────── R
 *              │                                   ║
 *              │                                   ║
 *              ●                                   ●
 *           (center)                     (center with wide stereo)
 *
 *     Only Mid content,                 Side boosted = wider image
 *     Side = 0                          Mid unchanged = center preserved
 *
 *
 *                    PROCESSING IN M/S DOMAIN
 *
 *
 *     Input (L/R)
 *          │
 *          ▼
 *     ┌──────────┐
 *     │  ENCODE  │   L/R → M/S
 *     │  to M/S  │
 *     └────┬─────┘
 *          │
 *     ┌────┴────┐
 *     │         │
 *     ▼         ▼
 *    Mid      Side
 *     │         │
 *     ▼         ▼
 *  [Process] [Process]   ← Independent processing
 *  (EQ, comp  (EQ, comp,    (different settings)
 *   gain)     width)
 *     │         │
 *     ▼         ▼
 *    Mid'     Side'
 *     │         │
 *     └────┬────┘
 *          │
 *          ▼
 *     ┌──────────┐
 *     │  DECODE  │   M/S → L/R
 *     │  to L/R  │
 *     └────┬─────┘
 *          │
 *          ▼
 *     Output (L/R)
 *
 * ============================================================================
 *                           ALGORITHM DETAILS
 * ============================================================================
 *
 * ENCODING FORMULA:
 *
 *   Mid  = (Left + Right) × 0.5
 *   Side = (Left - Right) × 0.5
 *
 * DECODING FORMULA:
 *
 *   Left  = Mid + Side
 *   Right = Mid - Side
 *
 * PROOF OF PERFECT RECONSTRUCTION:
 *
 * Given: M = (L+R)/2, S = (L-R)/2
 *
 * Decode Left:  M + S = (L+R)/2 + (L-R)/2 = (L+R+L-R)/2 = 2L/2 = L ✓
 * Decode Right: M - S = (L+R)/2 - (L-R)/2 = (L+R-L+R)/2 = 2R/2 = R ✓
 *
 * REFERENCES AND SCOPE
 *
 * M/S stereo encoding was documented by Alan Blumlein in 1933. This utility
 * implements the standard sum/difference matrix.
 *
 * References:
 * - Blumlein, A. (1933). British Patent 394,325
 * - Gerzon, M. (1994). "Optimum Reproduction Matrices for Multispeaker Stereo"
 * - Rumsey, F. (2001). "Spatial Audio" - Chapter on M/S techniques
 *
 * ============================================================================
 */

#include <bw_audio_types/BufferView.h>

namespace bws
{
namespace audio
{

//==============================================================================
/**
 * @brief Mid/Side stereo encoding and decoding utilities
 *
 * Provides stateless static functions to convert between Left/Right and
 * Mid/Side stereo representations. All methods are designed for zero-overhead
 * integration into audio processing loops.
 *
 * Mid = Center content (mono-compatible, appears in both L and R)
 * Side = Difference content (stereo width, appears differently in L and R)
 *
 * This is a static-only utility class - no instances needed.
 */
class MidSideProcessor
{
public:
    //==========================================================================
    // BUFFER ENCODING/DECODING
    //==========================================================================

    /**
     * @brief Encode stereo buffer from L/R to M/S in-place
     * @param buffer Stereo buffer (must have >= 2 channels)
     *
     * After encoding:
     *   Channel 0 = Mid (center content)
     *   Channel 1 = Side (stereo width)
     *
     * Formula:
     *   Mid  = (Left + Right) × 0.5
     *   Side = (Left - Right) × 0.5
     */
    static void encode(bws::domain::BufferView buffer) noexcept
    {
        // M/S encoding requires stereo
        if (buffer.numChannels < 2)
            return;

        float* left = buffer.channel(0);
        float* right = buffer.channel(1);
        const int numSamples = buffer.numSamples;

        // Encode each sample pair
        for (int i = 0; i < numSamples; ++i)
        {
            const float l = left[i];
            const float r = right[i];

            // Sum = center content (what's the same in L and R)
            left[i] = (l + r) * 0.5f; // Mid

            // Difference = stereo content (what's different between L and R)
            right[i] = (l - r) * 0.5f; // Side
        }
    }

    /**
     * @brief Scale stereo width by `widthScale` (M/S side scale) in L/R domain.
     *        0 = mono fold, 1 = unchanged, > 1 = super-stereo.
     *        Equivalent to encode + (side *= widthScale) + decode; collapsed
     *        to direct L/R math for fewer ops. Bypassed if mono.
     */
    static void applyWidth(bws::domain::BufferView buffer, float widthScale) noexcept
    {
        if (buffer.numChannels < 2)
            return;

        float* __restrict left = buffer.channel(0);
        float* __restrict right = buffer.channel(1);
        const int numSamples = buffer.numSamples;

        for (int i = 0; i < numSamples; ++i)
        {
            const float l = left[i];
            const float r = right[i];
            const float m = 0.5f * (l + r);
            const float s = 0.5f * (l - r) * widthScale;
            left[i] = m + s;
            right[i] = m - s;
        }
    }

    /**
     * @brief Decode stereo buffer from M/S back to L/R in-place
     * @param buffer Stereo buffer (must have >= 2 channels)
     *
     * Before decoding:
     *   Channel 0 = Mid
     *   Channel 1 = Side
     *
     * After decoding:
     *   Channel 0 = Left
     *   Channel 1 = Right
     *
     * Formula:
     *   Left  = Mid + Side
     *   Right = Mid - Side
     */
    static void decode(bws::domain::BufferView buffer) noexcept
    {
        if (buffer.numChannels < 2)
            return;

        float* mid = buffer.channel(0);
        float* side = buffer.channel(1);
        const int numSamples = buffer.numSamples;

        // Decode each sample pair
        for (int i = 0; i < numSamples; ++i)
        {
            const float m = mid[i];
            const float s = side[i];

            // Reconstruct left: center + difference
            mid[i] = m + s; // Left

            // Reconstruct right: center - difference
            side[i] = m - s; // Right
        }
    }

    //==========================================================================
    // SAMPLE-BY-SAMPLE ENCODING/DECODING
    //==========================================================================

    /**
     * @brief Encode a single stereo sample pair from L/R to M/S
     * @param left  Input left sample, output will be Mid
     * @param right Input right sample, output will be Side
     *
     * Parameters are modified in-place for zero-copy operation.
     */
    static void encodeSample(float& left, float& right) noexcept
    {
        const float l = left;
        const float r = right;
        left = (l + r) * 0.5f;  // Mid
        right = (l - r) * 0.5f; // Side
    }

    /**
     * @brief Decode a single stereo sample pair from M/S to L/R
     * @param mid  Input mid sample, output will be Left
     * @param side Input side sample, output will be Right
     *
     * Parameters are modified in-place for zero-copy operation.
     */
    static void decodeSample(float& mid, float& side) noexcept
    {
        const float m = mid;
        const float s = side;
        mid = m + s;  // Left
        side = m - s; // Right
    }

    //==========================================================================
    // SEPARATE MID/SIDE PROCESSING
    //==========================================================================

    /**
     * @brief Process Mid and Side channels with separate processors
     * @tparam ProcessFunc Callable type (lambda, function object, etc.)
     * @param buffer        Stereo buffer (modified in-place)
     * @param midProcessor  Processor for Mid channel (center content)
     * @param sideProcessor Processor for Side channel (stereo width)
     *
     * Workflow:
     * 1. Encode L/R to M/S
     * 2. Apply midProcessor to channel 0 (Mid)
     * 3. Apply sideProcessor to channel 1 (Side)
     * 4. Decode M/S back to L/R
     *
     * Example usage:
     * @code
     * // Compress mid harder than side for glue
     * MidSideProcessor::processSeparately(buffer,
     *     [](auto& b) { midCompressor.process(b); },
     *     [](auto& b) { sideCompressor.process(b); });
     *
     * // Boost side for wider stereo
     * MidSideProcessor::processSeparately(buffer,
     *     [](auto& b) { },  // Mid unchanged
     *     [](auto& b) { b.applyGain(1.5f); });  // Side +3.5dB
     * @endcode
     */
    template <typename ProcessFunc>
    static void processSeparately(bws::domain::BufferView buffer, ProcessFunc& midProcessor, ProcessFunc& sideProcessor)
    {
        // Encode L/R to M/S
        encode(buffer);

        // Create single-channel views for separate processing
        // This avoids copying - processors work directly on buffer memory
        // Use pointer arithmetic on the channels array to create sub-views
        bws::domain::BufferView midBuffer {buffer.channels, 1, buffer.numSamples};
        bws::domain::BufferView sideBuffer {buffer.numChannels > 1 ? buffer.channels + 1 : buffer.channels, 1,
                                            buffer.numSamples};

        // Apply separate processing
        midProcessor(midBuffer);
        sideProcessor(sideBuffer);

        // Decode M/S back to L/R
        decode(buffer);
    }

private:
    // Static-only class - constructor deleted to prevent instantiation
    MidSideProcessor() = delete;
};

} // namespace audio
} // namespace bws
