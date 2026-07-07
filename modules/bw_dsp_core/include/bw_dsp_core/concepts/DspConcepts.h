// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
/**
 * @file DspConcepts.h
 * @brief C++20 concepts for DSP module interface contracts
 *
 * Defines compile-time contracts for the three DSP module patterns
 * found across bw_dsp. These concepts describe what already exists -
 * they do NOT force API changes on existing modules.
 *
 * Usage:
 * static_assert(bws::dsp::PerSampleDSP<Processor>);
 *
 *   template<bws::dsp::PerSampleDSP Engine>
 *   void processEngine(Engine& e, float* buf, int n) { ... }
 */

#pragma once

#include <concepts>
#include <type_traits>
#include <bw_audio_types/BufferView.h>

namespace bws::dsp
{

// ---------------------------------------------------------------------------
// PerSampleDSP
// ---------------------------------------------------------------------------
// Modules that process one sample at a time: float in → float out.
// Initialization accepts setSampleRate(float) - the dominant pattern
// for per-sample processors.

template <typename T>
concept PerSampleDSP = requires(T& t, float sample, float sampleRate) {
    { t.setSampleRate(sampleRate) };
    { t.process(sample) } -> std::convertible_to<float>;
    { t.reset() };
};

// ---------------------------------------------------------------------------
// PerSampleDSPWithSidechain
// ---------------------------------------------------------------------------
// Extension of PerSampleDSP for processors that accept a separate control-source
// sample per call (the `detectorSource` argument). The two-arg process overload
// drives the internal envelope from the control source while applying its effect to
// `input`. Processors satisfying this concept also satisfy PerSampleDSP - a
// single-arg call uses the input as its own control source.
//
// anchorDetector(channel, signedSource) applies any control-path filtering that must
// see the signed signal per-channel; processors with no such filter return the input
// unchanged. processWithAnchoredDetector consumes a control the caller already
// anchored.

template <typename T>
concept PerSampleDSPWithSidechain = PerSampleDSP<T> && requires(T& t, float sample, float detectorSource, int channel) {
    { t.process(sample, detectorSource) } -> std::convertible_to<float>;
    { t.anchorDetector(channel, detectorSource) } -> std::convertible_to<float>;
    { t.processWithAnchoredDetector(sample, detectorSource, channel) } -> std::convertible_to<float>;
};

// ---------------------------------------------------------------------------
// BufferDSP
// ---------------------------------------------------------------------------
// Modules that process a full audio buffer in-place.
// Initialization accepts prepare(double sampleRate, int blockSize) -
// the dominant pattern for buffer-based processors.

template <typename T>
concept BufferDSP = requires(T& t, double sampleRate, int blockSize, bws::domain::BufferView buffer) {
    { t.prepare(sampleRate, blockSize) };
    { t.process(buffer) };
    { t.reset() };
};

// ---------------------------------------------------------------------------
// GainReductionReadable
// ---------------------------------------------------------------------------
// Modules that report gain reduction for metering.
//
// Note: modules may return gain reduction on a 0-1 scale or in dB.
// The concept verifies the method exists but cannot enforce units - that is
// the caller's responsibility.

template <typename T>
concept GainReductionReadable = requires(const T& t) {
    { t.getGainReduction() } -> std::convertible_to<float>;
};

} // namespace bws::dsp
