// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cmath>
#include <limits>
#include <algorithm>

namespace bws
{
namespace dsp
{

/**
 * DSP Constants and Configuration
 *
 * Centralized DSP constants used across all audio processing code.
 * This is the Single Source of Truth (SSoT) for DSP-related values.
 */
struct DspConstants
{
    // ─────────────────────────────────────────────────────────────────────────
    // Sample Rate Constraints
    // ─────────────────────────────────────────────────────────────────────────
    static constexpr double MIN_SAMPLE_RATE = 22050.0;
    static constexpr double MAX_SAMPLE_RATE = 192000.0;
    static constexpr double DEFAULT_SAMPLE_RATE = 44100.0;

    // ─────────────────────────────────────────────────────────────────────────
    // Buffer Size Constraints
    // ─────────────────────────────────────────────────────────────────────────
    static constexpr int MIN_BUFFER_SIZE = 1;
    static constexpr int MAX_BUFFER_SIZE = 8192;
    static constexpr int DEFAULT_BUFFER_SIZE = 512;

    // ─────────────────────────────────────────────────────────────────────────
    // Channel Limits
    // ─────────────────────────────────────────────────────────────────────────
    static constexpr int MAX_CHANNELS = 16;
    static constexpr int DEFAULT_CHANNELS = 2;

    // ─────────────────────────────────────────────────────────────────────────
    // Audio Range (dB)
    // ─────────────────────────────────────────────────────────────────────────
    static constexpr float MIN_DB = -120.0f;
    static constexpr float MAX_DB = 24.0f;
    static constexpr float SILENCE_THRESHOLD_DB = -96.0f;
    static constexpr float UNITY_GAIN_DB = 0.0f;

    // ─────────────────────────────────────────────────────────────────────────
    // Common Frequencies
    // ─────────────────────────────────────────────────────────────────────────
    static constexpr float NYQUIST_FACTOR = 0.5f;
    static constexpr float DC_FILTER_CUTOFF_HZ = 5.0f; // Standard DC blocking
    static constexpr float SUB_CUTOFF_HZ = 20.0f;      // Sub-bass filter
    static constexpr float AIR_CUTOFF_HZ = 16000.0f;   // Air band

    // ─────────────────────────────────────────────────────────────────────────
    // PI Constants (High precision)
    // ─────────────────────────────────────────────────────────────────────────
    static constexpr double PI = 3.14159265358979323846;
    static constexpr double TWO_PI = 6.28318530717958647692;
    static constexpr double HALF_PI = 1.57079632679489661923;
    static constexpr double QUARTER_PI = 0.78539816339744830962;

    // ─────────────────────────────────────────────────────────────────────────
    // Smoothing & Ramping
    // ─────────────────────────────────────────────────────────────────────────
    static constexpr double DEFAULT_RAMP_TIME_SEC = 0.02;    // 20ms - click-free
    static constexpr double PARAMETER_SMOOTHING_TIME = 0.05; // 50ms - UI smoothing
    static constexpr double FAST_RAMP_TIME_SEC = 0.005;      // 5ms - fast transitions
    static constexpr double SLOW_RAMP_TIME_SEC = 0.1;        // 100ms - smooth fades

    static constexpr double CROSSFADE_SECONDS = 512.0 / 48000.0; // 10.67ms - engine-switch crossfade

    // ─────────────────────────────────────────────────────────────────────────
    // Meter Ballistics
    // ─────────────────────────────────────────────────────────────────────────
    static constexpr double METER_ATTACK_MS = 1.0;    // Fast attack
    static constexpr double METER_RELEASE_MS = 300.0; // VU-style release
    static constexpr double PEAK_HOLD_MS = 1500.0;    // Peak hold time

    // ─────────────────────────────────────────────────────────────────────────
    // Golden Ratio (for UI/DSP proportions)
    // ─────────────────────────────────────────────────────────────────────────
    static constexpr double PHI = 1.618033988749895;
    static constexpr double PHI_INVERSE = 0.618033988749895;
    static constexpr double PHI_SQUARED = 2.618033988749895;

    // ─────────────────────────────────────────────────────────────────────────
    // Numeric Safety
    // ─────────────────────────────────────────────────────────────────────────
    static constexpr float DENORMAL_THRESHOLD = 1e-15f;
    static constexpr float MIN_GAIN_LINEAR = 1e-6f;    // ~-120dB
    static constexpr float MAX_GAIN_LINEAR = 15.8489f; // +24dB
};

/**
 * Common DSP Utility Functions
 *
 * Zero-overhead inline math utilities for audio processing.
 */
struct DspMath
{
    /** Convert linear gain to decibels */
    static inline float gainToDb(float gain) noexcept
    {
        return 20.0f * std::log10(std::max(gain, DspConstants::MIN_GAIN_LINEAR));
    }

    /** Convert decibels to linear gain */
    static inline float dbToGain(float db) noexcept { return std::pow(10.0f, db / 20.0f); }

    /** Clamp value to range */
    template <typename T>
    static constexpr T clamp(T value, T min, T max) noexcept
    {
        return std::max(min, std::min(max, value));
    }

    /** Linear interpolation */
    template <typename T>
    static constexpr T lerp(T a, T b, T t) noexcept
    {
        return a + t * (b - a);
    }

    /** Check if value is approximately zero (denormal prevention) */
    static inline bool isNearZero(float value) noexcept { return std::abs(value) < DspConstants::DENORMAL_THRESHOLD; }

    /** Flush denormals to zero */
    static inline float flushDenormal(float value) noexcept { return isNearZero(value) ? 0.0f : value; }

    /** Convert milliseconds to samples (clamped to int range). */
    static constexpr int msToSamples(double ms, double sampleRate) noexcept
    {
        constexpr double kMaxInt = static_cast<double>(std::numeric_limits<int>::max());
        constexpr double kMinInt = static_cast<double>(std::numeric_limits<int>::min());
        return static_cast<int>(clamp(ms * sampleRate / 1000.0, kMinInt, kMaxInt));
    }

    /** Convert samples to milliseconds */
    static constexpr double samplesToMs(int samples, double sampleRate) noexcept
    {
        return static_cast<double>(samples) * 1000.0 / sampleRate;
    }

    /** Calculate coefficient for one-pole smoothing */
    static inline float calcOnePoleCoeff(double timeConstantMs, double sampleRate) noexcept
    {
        return static_cast<float>(1.0 - std::exp(-1.0 / (timeConstantMs * 0.001 * sampleRate)));
    }
};

} // namespace dsp
} // namespace bws
