// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <array>
#include <atomic>
#include <cmath>
#include <algorithm>

namespace bws
{
namespace audio
{

#if defined(__clang__) && defined(__has_cpp_attribute) && defined(__has_warning)
#if __has_cpp_attribute(clang::nonblocking) && __has_warning("-Wfunction-effects")
#define BWS_TRUE_PEAK_NONBLOCKING [[clang::nonblocking]]
#endif
#endif

#ifndef BWS_TRUE_PEAK_NONBLOCKING
#define BWS_TRUE_PEAK_NONBLOCKING
#endif

//============================================================================
/**
 * @brief True Peak Meter with 4x oversampling for inter-sample peak detection
 *
 * ┌─────────────────────────────────────────────────────────────────────────┐
 * │                        SIGNAL FLOW DIAGRAM                              │
 * ├─────────────────────────────────────────────────────────────────────────┤
 * │                                                                         │
 * │  Input Audio (48kHz)                                                    │
 * │        │                                                                │
 * │        ▼                                                                │
 * │  ┌───────────────────┐                                                  │
 * │  │ Polyphase FIR     │  4x interpolation (49 taps, Hann-windowed sinc)  │
 * │  │ Upsampling        │                                                  │
 * │  └───────────────────┘                                                  │
 * │        │                                                                │
 * │        ▼                                                                │
 * │  Interpolated Signal (192kHz equivalent)                                │
 * │        │                                                                │
 * │        ▼                                                                │
 * │  ┌───────────────────┐                                                  │
 * │  │ Peak Detection    │  Find max |sample| across all 4 phases           │
 * │  │ (per channel)     │                                                  │
 * │  └───────────────────┘                                                  │
 * │        │                                                                │
 * │        ▼                                                                │
 * │  ┌───────────────────┐                                                  │
 * │  │ Hold & Decay      │  Configurable hold time + exponential decay      │
 * │  │ Envelope          │                                                  │
 * │  └───────────────────┘                                                  │
 * │        │                                                                │
 * │        ▼                                                                │
 * │  Atomic outputs → UI Thread                                             │
 * │                                                                         │
 * └─────────────────────────────────────────────────────────────────────────┘
 *
 * WHY TRUE PEAK MATTERS:
 * ----------------------
 * Digital audio samples represent discrete points in time. Between samples,
 * the reconstructed analog signal can exceed the sample values (inter-sample
 * peaks). A signal that peaks at -0.1 dBFS in samples might actually hit
 * +2 dBFS in the analog domain, causing clipping in D/A converters.
 *
 * True peak measurement uses oversampling to estimate these inter-sample peaks.
 * ITU-R BS.1770-5 specifies minimum 4x oversampling for broadcast compliance.
 *
 * ALGORITHM:
 * --------------------------
 * 1. Windowed Sinc Filter: sinc(x) = sin(πx)/(πx) with Hann window
 *    - Shannon 1949, Hann 1901, combined = textbook DSP
 *
 * 2. Polyphase Decomposition: Redistribute filter taps for efficiency
 *    - Crochiere & Rabiner 1983
 *
 * 3. Peak Hold/Decay: Simple envelope follower with timer
 *    - Basic signal processing
 *
 * INTEGRATION:
 * ------------
 * - Called from: the plugin's processBlock (end of chain)
 * - UI access via: the processor's getTruePeakDb() / getTruePeakHeldDb()
 */
template <int OversampleFactor_ = 4>
class TruePeakMeterImpl
{
public:
    //==========================================================================
    // CONSTANTS
    //==========================================================================

    // Oversampling factor (ITU-R BS.1770-5 minimum is 4×). 8× and 16× are
    // available for premium-tier ISP detection where the higher polyphase
    // resolution closes the residual gap to the +3.0103 dB analytic asymptote
    // on signals (e.g. fs/4 maximally-displaced-phase sine).
    static_assert(OversampleFactor_ == 4 || OversampleFactor_ == 8 || OversampleFactor_ == 16,
                  "TruePeakMeterImpl supports oversample factors 4, 8, or 16");

    // Maximum supported channels (stereo)
    static constexpr int kMaxChannels = 2;

    static constexpr int kOversampleFactor = OversampleFactor_;

    // Filter design. At 4× the lowpass kernel is 49 taps; at 8× and 16×
    // the kernel scales as
    // 12·OversampleFactor for uniform per-phase FIR length.
    static constexpr int kTotalTaps = (kOversampleFactor == 4) ? 49 : kOversampleFactor * 12;
    static constexpr int kTapsPerPhase = (kTotalTaps + kOversampleFactor - 1) / kOversampleFactor;

    // Default display parameters
    static constexpr float kDefaultHoldTimeMs = 2000.0f;      // 2 second hold before decay
    static constexpr float kDefaultDecayRateDbPerSec = 20.0f; // 20 dB/s decay rate

    TruePeakMeterImpl() = default;

    //==========================================================================
    /**
     * @brief Initialize meter for given sample rate
     *
     * MUST be called from prepareToPlay() before processing.
     * Generates polyphase filter coefficients and clears state.
     *
     * @param sampleRate Audio sample rate (44100, 48000, 96000, etc.)
     * @param numChannels Number of channels to process (1 or 2)
     *
     * COEFFICIENT GENERATION:
     * The filter coefficients are computed at runtime from the windowed sinc
     * formula.
     */
    void prepare(double sampleRate, int numChannels = 2) noexcept
    {
        sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
        numChannels_ = std::clamp(numChannels, 1, kMaxChannels);

        //----------------------------------------------------------------------
        // Generate polyphase filter coefficients using windowed sinc
        // This happens once at prepare time, not during processing
        //----------------------------------------------------------------------
        generateCoefficients();

        //----------------------------------------------------------------------
        // Clear filter delay lines (one per channel)
        //----------------------------------------------------------------------
        for (int ch = 0; ch < kMaxChannels; ++ch)
            for (int i = 0; i < kTapsPerPhase; ++i)
                delayLine_[ch][i] = 0.0f;

        delayWritePos_ = 0;

        // Reset all peak tracking state
        reset();
    }

    //==========================================================================
    /**
     * @brief Reset all peak values to minimum
     *
     * Call when:
     * - Transport stops/starts
     * - User requests meter reset
     * - Preset changes
     */
    void reset() noexcept
    {
        for (int ch = 0; ch < kMaxChannels; ++ch)
        {
            truePeakLin_[ch].store(0.0f, std::memory_order_relaxed);
            heldPeakLin_[ch].store(0.0f, std::memory_order_relaxed);
            holdCountdown_[ch] = 0;
        }
        maxTruePeakLin_.store(0.0f, std::memory_order_relaxed);
        maxHeldPeakLin_.store(0.0f, std::memory_order_relaxed);
    }

    //==========================================================================
    /**
     * @brief Set peak hold time before decay begins
     * @param ms Hold time in milliseconds (0 = no hold, instant decay)
     */
    void setHoldTimeMs(float ms) noexcept { holdTimeMs_ = std::max(0.0f, ms); }

    /**
     * @brief Set decay rate after hold expires
     * @param dbPerSec Decay rate in dB per second
     *                 20.0 = fast decay (good for detailed monitoring)
     *                 3.0 = slow decay (good for capturing peaks)
     */
    void setDecayRateDbPerSec(float dbPerSec) noexcept { decayRateDbPerSec_ = std::max(0.1f, dbPerSec); }

    //==========================================================================
    /**
     * @brief Process audio buffer and update true peak values
     *
     * SIGNAL FLOW (per sample):
     * ┌─────────────────────────────────────────────────────────────────┐
     * │  For each input sample:                                         │
     * │  1. Push sample into all channels' delay lines (same position)  │
     * │  2. For each channel:                                           │
     * │     a. Compute 4 interpolated samples using polyphase filter    │
     * │     b. Find max |value| across all 4 phases                     │
     * │     c. Update channel peak accumulator                          │
     * │  3. Advance delay line position (once, after all channels)      │
     * │                                                                 │
     * │  After all samples processed:                                   │
     * │  4. Update instantaneous peak (truePeakLin_)                    │
     * │  5. Update held peak with hold/decay logic (heldPeakLin_)       │
     * │  6. Update max across channels for stereo display               │
     * └─────────────────────────────────────────────────────────────────┘
     *
     * POLYPHASE INTERPOLATION:
     * Instead of upsampling then filtering (expensive), we use polyphase
     * decomposition. The filter is split into 4 sub-filters, each
     * producing one of the 4 interpolated samples directly.
     *
     * produces sample at t + 0/4
     * produces sample at t + 1/4
     * produces sample at t + 2/4
     * produces sample at t + 3/4
     *
     * @param buffer Audio buffer to analyze (const - meter doesn't modify)
     */
    template <typename BufferType>
    void process(const BufferType& buffer) noexcept BWS_TRUE_PEAK_NONBLOCKING
    {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = std::min(buffer.getNumChannels(), numChannels_);

        if (numSamples == 0 || numChannels == 0)
            return;

        //----------------------------------------------------------------------
        // Calculate hold/decay parameters for this block
        //----------------------------------------------------------------------
        const int holdSamples = static_cast<int>(holdTimeMs_ * 0.001f * static_cast<float>(sampleRate_));

        // Decay rate: convert dB/second to linear multiplier per sample
        // decayPerSample = dB_per_sec / sample_rate (dB per sample)
        // decayMultiplier = 10^(-dB_per_sample / 20) (linear multiplier)
        const float decayPerSample = decayRateDbPerSec_ / static_cast<float>(sampleRate_);
        const float decayMultiplier = std::pow(10.0f, -decayPerSample / 20.0f);

        float maxPeakThisBlock = 0.0f;

        // Per-block peak accumulators (find max across entire block)
        float channelPeaks[kMaxChannels] = {0.0f, 0.0f};

        //----------------------------------------------------------------------
        // MAIN PROCESSING LOOP: Sample-by-sample with all channels in sync
        //----------------------------------------------------------------------
        for (int i = 0; i < numSamples; ++i)
        {
            //------------------------------------------------------------------
            // STEP 1: Push input sample into each channel's delay line
            // All channels use the SAME delay line position for sync
            //------------------------------------------------------------------
            for (int ch = 0; ch < numChannels; ++ch)
            {
                const float* data = buffer.getReadPointer(ch);
                delayLine_[ch][delayWritePos_] = data[i];
            }

            //------------------------------------------------------------------
            // STEP 2: Compute interpolated peaks for each channel
            //------------------------------------------------------------------
            for (int ch = 0; ch < numChannels; ++ch)
            {
                // Compute all 4 interpolated samples (polyphase filtering)
                for (int phase = 0; phase < kOversampleFactor; ++phase)
                {
                    float interpolated = 0.0f;

                    // Convolve delay line with this phase's coefficients
                    // This is the polyphase FIR filter operation
                    for (int tap = 0; tap < kTapsPerPhase; ++tap)
                    {
                        // Read from delay line (circular buffer)
                        const int delayIdx = (delayWritePos_ - tap + kTapsPerPhase) % kTapsPerPhase;
                        interpolated += delayLine_[ch][delayIdx] * phaseCoeffs_[phase][tap];
                    }

                    // Track maximum absolute value across all phases
                    const float absPeak = std::abs(interpolated);
                    if (absPeak > channelPeaks[ch])
                        channelPeaks[ch] = absPeak;
                }
            }

            //------------------------------------------------------------------
            // STEP 3: Advance delay line position (AFTER all channels processed)
            // This ensures all channels read from consistent positions
            //------------------------------------------------------------------
            delayWritePos_ = (delayWritePos_ + 1) % kTapsPerPhase;
        }

        //----------------------------------------------------------------------
        // UPDATE PEAK TRACKING FOR EACH CHANNEL
        //----------------------------------------------------------------------
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float channelPeak = channelPeaks[ch];

            // Store instantaneous true peak (for real-time display)
            truePeakLin_[ch].store(channelPeak, std::memory_order_relaxed);

            //------------------------------------------------------------------
            // HELD PEAK: Implements hold time + exponential decay
            //
            // States:
            // 1. New peak detected → reset hold timer, update held value
            // 2. In hold period → maintain held value, count down
            // 3. Decay phase → apply exponential decay
            //------------------------------------------------------------------
            float heldPeak = heldPeakLin_[ch].load(std::memory_order_relaxed);

            if (channelPeak >= heldPeak)
            {
                // New peak - update and reset hold timer
                heldPeak = channelPeak;
                holdCountdown_[ch] = holdSamples;
            }
            else if (holdCountdown_[ch] > 0)
            {
                // Still in hold period - count down
                holdCountdown_[ch] -= numSamples;
            }
            else
            {
                // Hold expired - apply decay
                // Apply decay for each sample in block
                for (int s = 0; s < numSamples; ++s)
                    heldPeak *= decayMultiplier;

                // Flush denormals to zero (prevents CPU slowdown)
                if (heldPeak < 1e-10f)
                    heldPeak = 0.0f;
            }

            heldPeakLin_[ch].store(heldPeak, std::memory_order_relaxed);

            // Track max across channels for stereo display
            if (channelPeak > maxPeakThisBlock)
                maxPeakThisBlock = channelPeak;
        }

        //----------------------------------------------------------------------
        // UPDATE STEREO MAX VALUES
        //----------------------------------------------------------------------
        maxTruePeakLin_.store(maxPeakThisBlock, std::memory_order_relaxed);

        // Max held peak across channels
        float maxHeld = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float held = heldPeakLin_[ch].load(std::memory_order_relaxed);
            if (held > maxHeld)
                maxHeld = held;
        }
        maxHeldPeakLin_.store(maxHeld, std::memory_order_relaxed);
    }

    //==========================================================================
    // UI THREAD ACCESSORS (Thread-safe)
    //==========================================================================

    /**
     * @brief Get current true peak in dB (max across channels)
     * @return True peak level in dBFS (0 dB = full scale)
     */
    float getTruePeakDb() const noexcept
    {
        const float lin = maxTruePeakLin_.load(std::memory_order_relaxed);
        return linearToDb(lin);
    }

    /**
     * @brief Get held true peak in dB (with hold/decay, max across channels)
     * @return Held peak level in dBFS
     */
    float getHeldPeakDb() const noexcept
    {
        const float lin = maxHeldPeakLin_.load(std::memory_order_relaxed);
        return linearToDb(lin);
    }

    /**
     * @brief Get current true peak in dB for specific channel
     * @param channel Channel index (0 = left, 1 = right)
     */
    float getTruePeakDb(int channel) const noexcept
    {
        if (channel < 0 || channel >= kMaxChannels)
            return -100.0f;
        const float lin = truePeakLin_[channel].load(std::memory_order_relaxed);
        return linearToDb(lin);
    }

    /**
     * @brief Get held true peak in dB for specific channel
     */
    float getHeldPeakDb(int channel) const noexcept
    {
        if (channel < 0 || channel >= kMaxChannels)
            return -100.0f;
        const float lin = heldPeakLin_[channel].load(std::memory_order_relaxed);
        return linearToDb(lin);
    }

    /**
     * @brief Get true peak as linear amplitude (for meter drawing)
     * @return Linear amplitude (0.0 to ~1.0+, can exceed 1.0)
     */
    float getTruePeakLinear() const noexcept { return maxTruePeakLin_.load(std::memory_order_relaxed); }

    /**
     * @brief Reset held peak values (keeps instantaneous readings)
     */
    void resetHeldPeaks() noexcept
    {
        for (int ch = 0; ch < kMaxChannels; ++ch)
        {
            heldPeakLin_[ch].store(0.0f, std::memory_order_relaxed);
            holdCountdown_[ch] = 0;
        }
        maxHeldPeakLin_.store(0.0f, std::memory_order_relaxed);
    }

private:
    //==========================================================================
    // HELPER FUNCTIONS
    //==========================================================================

    /**
     * @brief Convert linear amplitude to decibels
     * @param linear Linear amplitude value
     * @return Value in dB (returns -100 for zero/negative input)
     */
    static float linearToDb(float linear) noexcept
    {
        if (linear <= 0.0f)
            return -100.0f;
        return 20.0f * std::log10(linear);
    }

    //==========================================================================
    /**
     * @brief Generate polyphase FIR coefficients using windowed sinc
     *
     * ALGORITHM (all public domain):
     *
     * 1. SINC FUNCTION (Shannon 1949, Nyquist 1928):
     *    sinc(x) = sin(π·x) / (π·x)  for x ≠ 0
     *    sinc(0) = 1
     *
     *    The sinc function is the ideal lowpass filter impulse response.
     *    Cutoff frequency = 1/factor (Nyquist of downsampled rate)
     *
     * 2. HANN WINDOW (Julius von Hann, 1901):
     *    w(n) = 0.5 · (1 - cos(2π·n/(N-1)))
     *
     *    Window function that reduces spectral leakage (sidelobes).
     *    Hann provides good balance of main lobe width vs sidelobe level.
     *
     * 3. POLYPHASE DECOMPOSITION (Crochiere & Rabiner, 1983):
     *    Instead of one big filter, split into 'factor' sub-filters.
     *    Phase p gets coefficients at indices: p, p+factor, p+2*factor, ...
     *
     *    This allows computing interpolated samples directly without
     *    actually upsampling the signal.
     *
     * COEFFICIENT STORAGE:
     *    phaseCoeffs_[phase][tap] where:
     *    - phase = 0..3 (for 4x oversampling)
     *    - tap = 0..12 (13 taps per phase from 49 total)
     */
    void generateCoefficients() noexcept
    {
        constexpr float pi = 3.14159265358979323846f;
        const int centerTap = kTotalTaps / 2; // = 24 (center of 49-tap filter)

        //----------------------------------------------------------------------
        // Generate the full lowpass kernel.
        //----------------------------------------------------------------------
        std::array<float, kTotalTaps> lowpassKernel {};

        for (int i = 0; i < kTotalTaps; ++i)
        {
            // Distance from center tap (for sinc calculation)
            const float n = static_cast<float>(i - centerTap);

            //------------------------------------------------------------------
            // SINC FUNCTION: sin(π·n/factor) / (π·n/factor)
            // Cutoff at Nyquist/factor ensures no aliasing when decimating
            //------------------------------------------------------------------
            float sinc;
            if (std::abs(n) < 1e-6f)
            {
                // Handle n=0 case: lim(x→0) sin(x)/x = 1
                sinc = 1.0f;
            }
            else
            {
                const float x = pi * n / static_cast<float>(kOversampleFactor);
                sinc = std::sin(x) / x;
            }

            //------------------------------------------------------------------
            // HANN WINDOW: 0.5 · (1 - cos(2π·i/(N-1)))
            // Smoothly tapers the filter to reduce spectral leakage
            //------------------------------------------------------------------
            const float window =
                0.5f * (1.0f - std::cos(2.0f * pi * static_cast<float>(i) / static_cast<float>(kTotalTaps - 1)));

            // Final coefficient = sinc * window
            lowpassKernel[static_cast<size_t>(i)] = sinc * window;
        }

        //----------------------------------------------------------------------
        // STEP 2: Decompose into polyphase components
        //
        // Phase p gets every 'factor'-th coefficient starting at p:
        // coeffs[0], coeffs[4], coeffs[8],.
        // coeffs[1], coeffs[5], coeffs[9],.
        // coeffs[2], coeffs[6], coeffs[10],.
        // coeffs[3], coeffs[7], coeffs[11],.
        //----------------------------------------------------------------------
        for (int phase = 0; phase < kOversampleFactor; ++phase)
        {
            for (int tap = 0; tap < kTapsPerPhase; ++tap)
            {
                const int protoIdx = phase + tap * kOversampleFactor;
                if (protoIdx < kTotalTaps)
                {
                    phaseCoeffs_[phase][tap] = lowpassKernel[static_cast<size_t>(protoIdx)];
                }
                else
                {
                    // Zero-pad if the kernel doesn't have enough coefficients.
                    phaseCoeffs_[phase][tap] = 0.0f;
                }
            }
        }
    }

    //==========================================================================
    // MEMBER VARIABLES
    //==========================================================================

    // Configuration (set in prepare())
    double sampleRate_ {48000.0};
    int numChannels_ {2};
    float holdTimeMs_ {kDefaultHoldTimeMs};
    float decayRateDbPerSec_ {kDefaultDecayRateDbPerSec};

    // Polyphase FIR coefficients (generated in prepare())
    // [phase][tap] = coefficient for that phase and tap
    float phaseCoeffs_[kOversampleFactor][kTapsPerPhase] {};

    // Polyphase FIR delay lines (one per channel)
    // Circular buffer storing recent input samples
    float delayLine_[kMaxChannels][kTapsPerPhase] {};
    int delayWritePos_ {0}; // Current write position (shared across channels)

    // Peak tracking (atomic for thread safety)
    std::atomic<float> truePeakLin_[kMaxChannels] {}; // Instantaneous true peak (linear)
    std::atomic<float> heldPeakLin_[kMaxChannels] {}; // Held peak with decay (linear)
    int holdCountdown_[kMaxChannels] {};              // Samples remaining in hold period

    // Max across channels (for stereo display)
    std::atomic<float> maxTruePeakLin_ {0.0f};
    std::atomic<float> maxHeldPeakLin_ {0.0f};
};

// Default 4× instantiation is the canonical TruePeakMeter - existing
// call sites (e.g. Barometer's Processor)
// keep their signatures unchanged via this alias.
using TruePeakMeter = TruePeakMeterImpl<4>;
using TruePeakMeter8x = TruePeakMeterImpl<8>;
using TruePeakMeter16x = TruePeakMeterImpl<16>;

} // namespace audio
} // namespace bws
