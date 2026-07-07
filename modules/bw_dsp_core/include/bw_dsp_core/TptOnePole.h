// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <bw_core/BwCompilerFeatures.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace bws::audio
{

/**
 * TptOnePole - state-preserving first-order TPT filter (highpass / lowpass).
 *
 * Pattern reference: Zavalishin's Topology-Preserving Transform (TPT) applied
 * to a 1-pole filter topology. Integrator state is the trapezoidally-integrated
 * capacitor voltage of the equivalent analog circuit and remains physically
 * meaningful across any coefficient change - eliminating filter-transient
 * artifacts on coefficient modulation by construction.
 *
 * Industry parallel for shape comparison: first-order TPT filters and
 * chowdsp::FirstOrderHPF/LPF. No source code copied; derived from the
 * Zavalishin / Cytomic mathematical formulation. Pattern C channel-handling
 * (dual API mono-default) matching in-tree LfAnchorFilter precedent.
 *
 * References:
 * - Zavalishin, V. (2012/2018). "The Art of VA Filter Design." (Native Instruments).
 *   Chapter 3, eqs 3.10-3.13.
 * - Simper, A. "Linear Trapezoidal Optimised SVF" + "OnePoleLinearLowPass"
 *   technical papers. cytomic.com/technical-papers
 *
 * Output equation per Zavalishin Ch. 3 §3.10-3.13:
 *     v  = (input - s) * G         // G = g / (1 + g),  g = tan(pi * fc / fs)
 *     lp = v + s
 *     hp = input - lp
 *     s  = lp + v                  // state update; subnormal-flushed
 *
 * Location: lives in modules/bw_dsp_core/ as the shared one-pole TPT
 * primitive used by multiple DSP modules.
 *
 * Thread-safety:
 *   All setters (setCutoffHz, setType, prepare) must be called from the SAME
 *   thread as processSample. No internal synchronization. Cross-thread parameter
 *   updates require external smoothing.
 *
 * Subnormal handling:
 *   Callers SHOULD enable FTZ/DAZ at the audio-callback boundary. As
 *   belt-and-suspenders, this primitive also performs a manual flush after
 *   each state update, costing ~1 cycle.
 *
 * User-observable behavior:
 *   Bypass→reactivate does NOT clear integrator state. Steady-state DC is
 *   preserved across mid-audio reset paths per the audible-reality state
 * persistence doctrine.
 *   Only true cold-start (field initializers) zeros state. If a product needs
 *   Bypass-clears-state behavior, route through a per-engine wrapper,
 *   not by adding reset() to this primitive.
 *
 * RT-safety:
 *   processSample is allocation-free, branch-predictable, header-inlinable.
 *   prepare/setCutoffHz invoke std::tan (~30-50 cycles); safe per-block, safe
 *   per-sample but not optimized for sample-accurate automation.
 *
 * Note: BW_ALWAYS_INLINE maps to the per-compiler force-inline attribute.
 *
 * Floating-point precision notes:
 *   - HP output at fc << f_input exhibits residual ~10·machine_epsilon·|input|
 *     due to the cancellation hp = input - lp. Inaudible (<-120 dBFS at unity
 *     input); a TPT topology property, not a defect.
 *   - Coefficient state (g, G, sampleRate, cutoffHz) held in double regardless
 *     of sample type. Avoids precision floor at fc=10Hz (only 3 significant
 *     float-32 digits otherwise).
 */
class TptOnePole
{
public:
    enum class Type : std::uint8_t
    {
        Highpass,
        Lowpass
    };

    static constexpr int kMaxChannels = 2;
    static constexpr float kMinCutoffHz = 1.0f;
    static constexpr float kNyquistGuard = 0.99f;
    static constexpr float kInputClamp = 100.0f;
    static constexpr float kSubnormalFlush = 1e-20f;
    static constexpr double kDefaultSampleRate = 48000.0;
    static constexpr double kDefaultCutoffHz = 100.0;

    TptOnePole() noexcept
    {
        // E8: default-constructed filter is a valid 100Hz HP at 48kHz from
        // cycle zero. std::tan is not constexpr in C++20, so we compute at
        // construction rather than at field-init.
        updateCoefficient();
    }

    // Lifecycle. RT-safe (no allocation). MAY be called from audio thread.
    // Recomputes coefficients but does NOT zero state - state-preservation
    // contract per audible_reality_state_persistence.md.
    void prepare(double sampleRate, int numChannels = kMaxChannels) noexcept
    {
        sampleRate_ = (sampleRate < 1.0) ? kDefaultSampleRate : sampleRate;
        numChannels_ = std::clamp(numChannels, 1, kMaxChannels);
        updateCoefficient();
    }

    void setType(Type t) noexcept { type_ = t; }

    void setCutoffHz(float fc) noexcept
    {
        const double nyquistMax = sampleRate_ * 0.5 * static_cast<double>(kNyquistGuard);
        const double clamped = std::clamp(static_cast<double>(fc), static_cast<double>(kMinCutoffHz), nyquistMax);
        cutoffHz_ = clamped;
        updateCoefficient();
    }

    // Per-sample. Arg order (channel, input) matches LfAnchorFilter precedent.
    // OOB channel clamps to [0, numChannels-1]. NaN/Inf input clamped to
    // [-kInputClamp, kInputClamp] before processing.
    BW_ALWAYS_INLINE float processSample(float input) noexcept { return processSample(0, input); }

    BW_ALWAYS_INLINE float processSample(int channel, float input) noexcept
    {
        const int ch = std::clamp(channel, 0, numChannels_ - 1);
        const float x = sanitize(input);
        const float s = s1_[static_cast<size_t>(ch)];
        const float v = static_cast<float>((static_cast<double>(x - s)) * G_);
        const float lp = v + s;
        const float hp = x - lp;
        const float newState = flushSubnormal(lp + v);
        s1_[static_cast<size_t>(ch)] = newState;
        return flushSubnormal((type_ == Type::Highpass) ? hp : lp);
    }

    float getCutoffHz() const noexcept { return static_cast<float>(cutoffHz_); }
    Type getType() const noexcept { return type_; }

    // NOTE: no public reset(). State preservation is the contract.
    // See audible_reality_state_persistence.md; modification of this rule
    // requires updating the persistence doc + reviewing all adopters.

private:
    void updateCoefficient() noexcept
    {
        constexpr double kPi = 3.14159265358979323846;
        const double g = std::tan(kPi * cutoffHz_ / sampleRate_);
        G_ = g / (1.0 + g);
    }

    BW_ALWAYS_INLINE static float sanitize(float x) noexcept
    {
        return std::isfinite(x) ? std::clamp(x, -kInputClamp, kInputClamp) : 0.0f;
    }

    BW_ALWAYS_INLINE static float flushSubnormal(float x) noexcept
    {
        return (std::fabs(x) < kSubnormalFlush) ? 0.0f : x;
    }

    // Hot fields first (R3-L1 amendment): G_, s1_, type_, numChannels_.
    // Cold tail: sampleRate_, cutoffHz_.
    double G_ {0.006502439}; // tan(pi*100/48000) / (1+tan(...)); overwritten by ctor
    std::array<float, kMaxChannels> s1_ {};
    Type type_ {Type::Highpass};
    int numChannels_ {kMaxChannels};
    double sampleRate_ {kDefaultSampleRate};
    double cutoffHz_ {kDefaultCutoffHz};
};

} // namespace bws::audio
