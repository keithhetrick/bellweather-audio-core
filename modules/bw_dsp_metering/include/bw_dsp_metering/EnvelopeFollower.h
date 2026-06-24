// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <bw_dsp_core/DspConstants.h>
/**
 * ============================================================================
 * ENVELOPE FOLLOWER MODULE
 * ============================================================================
 *
 * Professional envelope detection and analysis for dynamics processing.
 *
 * USE CASES:
 * - Compressor/Limiter: Extract signal level for gain reduction
 * - Noise Gate: Track signal for open/close decisions
 * - Ducking: Follow voice level to control music attenuation
 * - Auto-gain: Track average level for automatic normalization
 * - Modulation: Create control signals from audio for synth parameters
 * - Transient Shaping: Identify attack portions of sounds
 *
 * ============================================================================
 *                              ARCHITECTURE
 * ============================================================================
 *
 *                    ENVELOPE FOLLOWER SIGNAL FLOW
 *
 *
 *     Audio Input
 *           │
 *           ▼
 *     ┌───────────────┐
 *     │   RECTIFIER   │   Convert to absolute value (full-wave rectification)
 *     │    |x(n)|     │
 *     └───────┬───────┘
 *             │
 *             ▼
 *     ┌───────────────────────────────────────────────────────────┐
 *     │              ASYMMETRIC LOWPASS FILTER                    │
 *     │                                                           │
 *     │   if |x| > envelope:        if |x| <= envelope:           │
 *     │       use ATTACK coeff          use RELEASE coeff         │
 *     │       (fast tracking)           (slow decay)              │
 *     │                                                           │
 *     │   y[n] = (1-α) × |x| + α × y[n-1]                         │
 *     │                                                           │
 *     │   where α = attackCoeff or releaseCoeff                   │
 *     └───────────────────────────┬───────────────────────────────┘
 *                                 │
 *                                 ▼
 *                         Envelope Output
 *                          (0.0 to 1.0+)
 *
 *
 *                    ATTACK vs RELEASE BEHAVIOR
 *
 *
 *     Input Signal:
 *
 *     Level │    ╱╲              ╱╲
 *           │   ╱  ╲            ╱  ╲
 *           │  ╱    ╲          ╱    ╲
 *           │ ╱      ╲        ╱      ╲
 *           │╱        ╲──────╱        ╲──────
 *           └─────────────────────────────────► Time
 *
 *     Envelope Output (fast attack, slow release):
 *
 *     Level │    ╱─╲              ╱─╲
 *           │   ╱   ╲            ╱   ╲
 *           │  ╱     ╲          ╱     ╲
 *           │ ╱       ╲        ╱       ╲
 *           │╱         ╲──────╱         ╲────
 *           └─────────────────────────────────► Time
 *                    ↑         ↑
 *               Fast Attack   Slow Release
 *               (tracks up)   (decays down)
 *
 *
 *                    TRANSIENT DETECTOR PRINCIPLE
 *
 *
 *     Uses DIFFERENTIAL between fast and slow envelopes:
 *
 *     Input (drum hit):
 *
 *           │    ▲
 *           │   ╱│╲
 *           │  ╱ │ ╲
 *           │ ╱  │  ╲───────
 *           │╱   │
 *           └────┼────────────────► Time
 *                │
 *                └── Transient (fast rise)
 *
 *     Fast Envelope (0.5ms attack):
 *
 *           │    ╱─╲
 *           │   ╱   ╲
 *           │  ╱     ╲
 *           │ ╱       ╲──────
 *           │╱
 *           └─────────────────► Time
 *
 *     Slow Envelope (20ms attack):
 *
 *           │      ╱────────
 *           │     ╱
 *           │    ╱
 *           │   ╱
 *           │──╱
 *           └─────────────────► Time
 *
 *     Differential (Fast - Slow):
 *
 *           │   ╱╲
 *           │  ╱  ╲
 *           │ ╱    ╲
 *           │╱      ╲────────   (returns to zero after transient)
 *           └─────────────────► Time
 *               ↑
 *          TRANSIENT DETECTED
 *         (differential > threshold)
 *
 * ============================================================================
 *                           ALGORITHM DETAILS
 * ============================================================================
 *
 * EXPONENTIAL MOVING AVERAGE (EMA):
 *
 * The envelope follower uses a one-pole IIR lowpass filter:
 *
 *   y[n] = (1 - α) × x[n] + α × y[n-1]
 *
 * Where:
 *   x[n] = rectified input sample
 *   y[n] = envelope output
 *   α    = smoothing coefficient (0.0 to 1.0)
 *
 * TIME CONSTANT CONVERSION:
 *
 * The coefficient α determines the time constant τ:
 *
 *   α = exp(-1 / (τ × sampleRate))
 *
 * Where τ is the time in seconds for the envelope to reach ~63% of target.
 * Common values:
 *   - Attack:  1-30ms (fast response to transients)
 *   - Release: 50-500ms (smooth decay to avoid pumping)
 *
 * REFERENCES AND SCOPE
 *
 * This header implements common signal-processing primitives:
 *
 * 1. FULL-WAVE RECTIFICATION: |x| - basic absolute value
 *
 * 2. EXPONENTIAL MOVING AVERAGE: One-pole IIR lowpass
 *    - Documented since 1950s (Smith, "Digital Signal Processing")
 *    - Standard DSP textbook material
 *
 * 3. DIFFERENTIAL ENVELOPE: Fast - Slow for transient detection
 *    - Basic arithmetic operation
 *    - Technique documented in AES papers since 1970s
 *
 * References:
 * - Smith, J.O. "Introduction to Digital Filters" - Chapter on EMA
 * - Zölzer, U. "DAFX: Digital Audio Effects" - Chapter 4
 * - Giannoulis et al. (2012). "Digital Dynamic Range Compressor Design"
 *
 * ============================================================================
 */

#include <bw_audio_types/ConstBufferView.h>
#include <chrono>

namespace bws::audio
{

//==============================================================================
/**
 * @brief Professional envelope follower with asymmetric attack/release
 *
 * Extracts the amplitude envelope from an audio signal using an asymmetric
 * lowpass filter. Different coefficients for rising vs falling signals
 * allow independent control of attack (how fast it responds to transients)
 * and release (how fast it decays after peaks).
 */
class EnvelopeFollower
{
public:
    //==========================================================================
    // CONSTRUCTION
    //==========================================================================

    EnvelopeFollower() = default;

    /**
     * @brief Construct with specific attack/release times
     * @param attackTimeMs  Time to reach ~63% of peak (typically 1-30ms)
     * @param releaseTimeMs Time to decay to ~37% of peak (typically 50-500ms)
     */
    EnvelopeFollower(float attackTimeMs, float releaseTimeMs)
        : attackMs(attackTimeMs)
        , releaseMs(releaseTimeMs)
    {
        updateCoefficients();
    }

    //==========================================================================
    // PREPARATION
    //==========================================================================

    void prepare(double newSampleRate, int /*blockSize*/ = 0)
    {
        sampleRate = newSampleRate;
        updateCoefficients();
    }

    //==========================================================================
    // PARAMETER CONTROL
    //==========================================================================

    /** Set attack time in milliseconds (how fast envelope rises) */
    void setAttackMs(float ms)
    {
        attackMs = ms;
        updateCoefficients();
    }

    /** Set release time in milliseconds (how fast envelope falls) */
    void setReleaseMs(float ms)
    {
        releaseMs = ms;
        updateCoefficients();
    }

    /** Alias for setAttackMs for API consistency */
    void setAttackTime(float ms) { setAttackMs(ms); }

    /** Alias for setReleaseMs for API consistency */
    void setReleaseTime(float ms) { setReleaseMs(ms); }

    //==========================================================================
    // PROCESSING
    //==========================================================================

    /**
     * @brief Process an entire audio buffer and return final envelope value
     * @param buffer Stereo or mono audio buffer
     * @return The envelope level after processing the entire buffer
     *
     * Processes all samples, tracking the peak across all channels.
     * Useful when you need the envelope value at the end of a block.
     */
    float process(bws::domain::ConstBufferView buffer) noexcept
    {
        const int numSamples = buffer.numSamples;
        const int numChannels = buffer.numChannels;

        // Process each sample, finding peak across all channels
        for (int i = 0; i < numSamples; ++i)
        {
            // Find the maximum absolute value across all channels for this sample
            float peak = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch)
                peak = std::max(peak, std::abs(buffer.channel(ch)[i]));

            // Update envelope with this peak value
            processSample(peak);
        }

        return currentEnvelope;
    }

    /**
     * @brief Process a single sample through the envelope follower
     * @param input Input sample (will be rectified internally)
     * @return Current envelope level
     *
     * Core envelope algorithm:
     * 1. Rectify input (take absolute value)
     * 2. Compare to current envelope
     * 3. If input > envelope: use attack coefficient (fast rise)
     * 4. If input <= envelope: use release coefficient (slow decay)
     * 5. Apply one-pole lowpass: y = (1-α)×x + α×y
     */
    float processSample(float input) noexcept
    {
        // Full-wave rectification: convert to positive amplitude
        const float x = std::abs(input);

        // Asymmetric smoothing: fast attack, slow release
        // Choose coefficient based on whether envelope is rising or falling
        const float coeff = (x > currentEnvelope) ? attackCoeff : releaseCoeff;

        // One-pole IIR lowpass: y[n] = (1-α)×x[n] + α×y[n-1]
        // This is the exponential moving average formula
        currentEnvelope = (1.0f - coeff) * x + coeff * currentEnvelope;

        return currentEnvelope;
    }

    // PerSampleDSP concept shim - forwards to processSample
    float process(float x) noexcept { return processSample(x); }

    //==========================================================================
    // STATE ACCESS
    //==========================================================================

    /**
     * @brief Get current envelope level
     * @return Envelope value (typically 0.0 to 1.0, but can exceed 1.0)
     *
     * Returns the most recent envelope value without processing.
     * Useful for UI meters or control logic.
     */
    float getCurrentLevel() const { return currentEnvelope; }

    /** Reset envelope to zero (call when audio stream restarts) */
    void reset() { currentEnvelope = 0.0f; }

private:
    //==========================================================================
    // COEFFICIENT CALCULATION
    //==========================================================================

    /**
     * @brief Recalculate filter coefficients from time constants
     *
     * Converts attack/release times (in ms) to filter coefficients.
     * Formula: α = exp(-1 / (τ × sampleRate))
     * where τ is the time constant in seconds.
     */
    void updateCoefficients()
    {
        if (sampleRate <= 0.0)
            return;
        // Convert ms to seconds, with minimum to avoid division by zero
        const float attackSeconds = std::max(0.0001f, attackMs * 0.001f);
        const float releaseSeconds = std::max(0.0001f, releaseMs * 0.001f);

        // Calculate coefficients: α = exp(-1 / (τ × sr))
        // Higher α = slower response (more smoothing)
        // Lower α = faster response (less smoothing)
        attackCoeff = std::exp(-1.0f / (attackSeconds * static_cast<float>(sampleRate)));
        releaseCoeff = std::exp(-1.0f / (releaseSeconds * static_cast<float>(sampleRate)));
    }

    //==========================================================================
    // MEMBER VARIABLES
    //==========================================================================

    double sampleRate = 44100.0;  ///< Current sample rate in Hz
    float attackMs = 5.0f;        ///< Attack time in milliseconds
    float releaseMs = 50.0f;      ///< Release time in milliseconds
    float attackCoeff = 0.0f;     ///< Calculated attack coefficient (0-1)
    float releaseCoeff = 0.0f;    ///< Calculated release coefficient (0-1)
    float currentEnvelope = 0.0f; ///< Current envelope state
};


//==============================================================================
/**
 * @brief Transient detector using differential envelope analysis
 *
 * Identifies percussive hits and transients by comparing fast and slow
 * envelopes. When the fast envelope significantly exceeds the slow envelope,
 * a transient is detected.
 *
 * The differential approach is self-calibrating: it responds to relative
 * changes rather than absolute levels, making it robust across different
 * input volumes.
 */
class TransientDetector
{
public:
    //==========================================================================
    // PREPARATION
    //==========================================================================

    /**
     * @brief Prepare for playback
     * @param newSampleRate Audio sample rate in Hz
     * - blockSize:     (unused) Maximum expected block size
     */
    void prepare(double newSampleRate, int /*blockSize*/)
    {
        sampleRate = newSampleRate;

        // ====================================================================
        // FAST ENVELOPE: Tracks transient peaks closely
        // ====================================================================
        // Very quick attack (0.5ms): responds almost instantly to peaks
        // Quick release (5ms): follows the attack portion tightly
        fastEnvelope.prepare(sampleRate);
        fastEnvelope.setAttackMs(0.5f);
        fastEnvelope.setReleaseMs(5.0f);

        // ====================================================================
        // SLOW ENVELOPE: Tracks sustained/average level
        // ====================================================================
        // Slower attack (20ms): ignores fast transients, tracks sustained level
        // Slower release (100ms): smooth, stable baseline
        slowEnvelope.prepare(sampleRate);
        slowEnvelope.setAttackMs(20.0f);
        slowEnvelope.setReleaseMs(100.0f);

        reset();
    }

    //==========================================================================
    // PARAMETER CONTROL
    //==========================================================================

    /**
     * @brief Set detection sensitivity
     * @param sens Sensitivity value (0.0 = detect all, 1.0 = only strong)
     *
     * Higher sensitivity means higher threshold, so only strong transients
     * are detected. Lower sensitivity detects more subtle transients.
     */
    void setSensitivity(float sens) { sensitivity = bws::dsp::DspMath::clamp(sens, 0.0f, 1.0f); }

    //==========================================================================
    // OUTPUT STRUCTURE
    //==========================================================================

    /**
     * @brief Information about detected transients
     */
    struct TransientInfo
    {
        bool isTransient = false; ///< Was a transient detected this block?
        float strength = 0.0f;    ///< Transient strength (0.0 to 1.0)
        int samplePosition = 0;   ///< Sample index where transient occurred
    };

    //==========================================================================
    // PROCESSING
    //==========================================================================

    /**
     * @brief Process a block and detect transients
     * @param buffer Audio buffer to analyze
     * @return TransientInfo with detection results
     *
     * Algorithm:
     * 1. Track signal with both fast and slow envelopes
     * 2. Calculate differential: fast - slow
     * 3. If differential exceeds threshold, transient detected
     * 4. Return position and strength of strongest transient in block
     */
    TransientInfo process(bws::domain::ConstBufferView buffer) noexcept
    {
        TransientInfo result;
        result.isTransient = false;
        result.strength = 0.0f;
        result.samplePosition = 0;

        const int numSamples = buffer.numSamples;
        const int numChannels = buffer.numChannels;

        // ====================================================================
        // ADAPTIVE THRESHOLD
        // ====================================================================
        // Threshold scales with sensitivity parameter
        // Range: 0.02 (sens=0, detect most) to 0.3 (sens=1, only strong)
        const float adaptiveThreshold = 0.02f + sensitivity * 0.28f;

        float maxDifferential = 0.0f;
        int maxPosition = 0;

        // Process each sample
        for (int i = 0; i < numSamples; ++i)
        {
            // Find peak across all channels for this sample
            float peak = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch)
                peak = std::max(peak, std::abs(buffer.channel(ch)[i]));

            // Track with both envelope followers
            const float fastLevel = fastEnvelope.processSample(peak);
            const float slowLevel = slowEnvelope.processSample(peak);

            // ================================================================
            // DIFFERENTIAL CALCULATION
            // ================================================================
            // The key insight: transients cause the fast envelope to
            // temporarily exceed the slow envelope. The difference
            // indicates how "sudden" the level change is.
            const float differential = fastLevel - slowLevel;

            // Track the maximum differential in this block
            if (differential > maxDifferential)
            {
                maxDifferential = differential;
                maxPosition = i;
            }
        }

        // ====================================================================
        // TRANSIENT DETECTION
        // ====================================================================
        // If differential exceeds threshold, we detected a transient
        if (maxDifferential > adaptiveThreshold)
        {
            result.isTransient = true;
            result.samplePosition = maxPosition;

            // Normalize strength to 0.0-1.0 range
            // Maps (threshold to 0.5) differential to (0.0 to 1.0) strength
            result.strength = bws::dsp::DspMath::clamp(
                (maxDifferential - adaptiveThreshold) / (0.5f - adaptiveThreshold), 0.0f, 1.0f);
        }

        return result;
    }

    /** Reset both envelope followers (call when audio stream restarts) */
    void reset()
    {
        fastEnvelope.reset();
        slowEnvelope.reset();
    }

private:
    EnvelopeFollower fastEnvelope; ///< Quick-responding envelope
    EnvelopeFollower slowEnvelope; ///< Slow baseline envelope
    double sampleRate = 44100.0;   ///< Current sample rate
    float sensitivity = 0.5f;      ///< Detection sensitivity (0-1)
};


// ============================================================================
// Mid/Side processing lives in MidSideProcessor.h
// Use: #include <bw_dsp_metering/MidSideProcessor.h>
// API: MidSideProcessor::encode(), decode(), encodeSample(), decodeSample(),
//      processSeparately()
// ============================================================================


//==============================================================================
/**
 * @brief CPU overload detector with safe-mode fallback
 *
 * Monitors real-time audio processing performance and detects when the
 * plugin is approaching or exceeding the real-time deadline. Can trigger
 * "safe mode" to reduce CPU usage when overloads occur.
 *
 * Real-time audio has a hard deadline: if processBlock() takes longer
 * than the time duration of the buffer, audio will glitch. This class
 * tracks those deadlines and detects patterns of overload.
 */
class CPUOverloadDetector
{
public:
    //==========================================================================
    // PREPARATION
    //==========================================================================

    /**
     * @brief Prepare for monitoring
     * @param newSampleRate  Audio sample rate in Hz
     * @param maxBlockSize   Maximum expected block size
     *
     * Calculates the real-time deadline: how long we have to process
     * each block before audio glitches occur.
     */
    void prepare(double newSampleRate, int maxBlockSize)
    {
        sampleRate = newSampleRate;

        // ====================================================================
        // DEADLINE CALCULATION
        // ====================================================================
        // The real-time deadline is the time duration of the audio block.
        // If we take longer than this to process, the audio thread starves.
        //
        // Example: 512 samples @ 48kHz = 10.67ms deadline
        const double blockDurationSeconds = static_cast<double>(maxBlockSize) / sampleRate;
        allowedTimeMs = static_cast<float>(blockDurationSeconds * 1000.0);

        // No per-prepare timing setup needed - std::chrono handles resolution

        reset();
    }

    //==========================================================================
    // TIMING API
    //==========================================================================

    /**
     * @brief Call at the START of processBlock
     *
     * Records the high-resolution timestamp when processing begins.
     * Must be paired with endBlock() call.
     */
    void startBlock() noexcept { blockStartTime = std::chrono::high_resolution_clock::now(); }

    /**
     * @brief Call at the END of processBlock
     * @param numSamples Actual number of samples processed this block
     *
     * Calculates processing time, updates CPU load estimate, and tracks
     * overload events.
     */
    void endBlock(int numSamples) noexcept
    {
        // Get end timestamp
        const auto endTime = std::chrono::high_resolution_clock::now();
        const auto elapsed = endTime - blockStartTime;

        // Convert elapsed duration to milliseconds
        const float elapsedMs = std::chrono::duration<float, std::milli>(elapsed).count();

        // ====================================================================
        // ACTUAL DEADLINE FOR THIS BLOCK
        // ====================================================================
        // The actual allowed time may differ from maxBlockSize if the host
        // sends smaller blocks (common during automation, transport, etc.)
        const float actualAllowedMs = static_cast<float>(static_cast<double>(numSamples) / sampleRate * 1000.0);

        // ====================================================================
        // CPU LOAD CALCULATION
        // ====================================================================
        // Express as fraction of deadline: 0.0 = idle, 1.0 = 100% of deadline
        // Values > 1.0 indicate overload (processing took longer than allowed)
        const float instantLoad = (actualAllowedMs > 0.0f) ? (elapsedMs / actualAllowedMs) : 0.0f;

        // ====================================================================
        // EXPONENTIAL SMOOTHING
        // ====================================================================
        // Smooth the CPU load reading to avoid jitter in UI displays
        // Factor of 0.3 gives responsive but stable readings
        constexpr float smoothingFactor = 0.3f;
        currentCPULoad = currentCPULoad + smoothingFactor * (instantLoad - currentCPULoad);

        // ====================================================================
        // OVERLOAD TRACKING
        // ====================================================================
        // Track consecutive overloads to detect sustained problems
        // (vs occasional spikes that might be unavoidable)
        if (instantLoad > 1.0f)
        {
            // Increment overload count (capped for stability)
            if (overloadCount < maxOverloads)
                ++overloadCount;
        }
        else if (overloadCount > 0)
        {
            // Slowly recover from overload state when running smoothly
            // Only recover when well under deadline (80%) to prevent thrashing
            if (instantLoad < 0.8f)
                --overloadCount;
        }
    }

    //==========================================================================
    // STATE QUERIES
    //==========================================================================

    /**
     * @brief Check if safe mode should be activated
     * @return true if multiple consecutive overloads detected
     *
     * Returns true when sustained overloads suggest the plugin should
     * reduce CPU usage (disable features, lower quality, etc.)
     */
    bool shouldActivateSafeMode() const { return overloadCount > 3; }

    /**
     * @brief Get smoothed CPU load estimate
     * @return CPU load as fraction (0.0 = idle, 1.0 = 100% of deadline)
     */
    float getCPULoad() const { return currentCPULoad; }

    /**
     * @brief Get current overload counter
     * @return Number of consecutive overload events
     */
    int getOverloadCount() const { return overloadCount; }

    /** Reset all state (call when audio stream restarts) */
    void reset()
    {
        overloadCount = 0;
        currentCPULoad = 0.0f;
        blockStartTime = {};
    }

private:
    double sampleRate = 44100.0;                                      ///< Current sample rate
    std::chrono::high_resolution_clock::time_point blockStartTime {}; ///< Timestamp at block start
    float allowedTimeMs = 0.0f;                                       ///< Real-time deadline in ms
    float currentCPULoad = 0.0f;                                      ///< Smoothed CPU load (0-1+)
    int overloadCount = 0;                                            ///< Consecutive overload counter

    static constexpr int maxOverloads = 5; ///< Max tracked overloads
};

} // namespace bws::audio
