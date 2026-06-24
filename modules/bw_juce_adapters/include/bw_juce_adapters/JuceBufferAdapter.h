// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include <bw_core/BwCompilerFeatures.h>

#include <bw_audio_types/BufferView.h>
#include <bw_audio_types/ConstBufferView.h>
#include <bw_juce_adapters/juce_overrides.h> // FEA-suppressed JUCE surface (audio-thread)

namespace bws::adapters
{

/**
 * Zero-copy adapter: juce::AudioBuffer<float> → bws::domain::BufferView.
 *
 * Points directly into the AudioBuffer's internal channel pointer array.
 * Valid as long as the source AudioBuffer is alive and not resized -
 * always true within a processBlock scope.
 *
 *
 * and the thread_local bug this approach avoids.
 */
// R5 BW_LIFETIMEBOUND: the returned view's channel pointers reference
// data INSIDE `buf`; the view's lifetime is bound to buf's lifetime. Clang
// surfaces dangling-pointer warnings via -Wdangling when an ephemeral buffer
// is passed (e.g., `auto v = toBufferView(make_ephemeral_buffer());`).
//
inline bws::domain::BufferView toBufferView(juce::AudioBuffer<float>& buf BW_LIFETIMEBOUND) noexcept
{
    return {buf.getArrayOfWritePointers(), buf.getNumChannels(), buf.getNumSamples()};
}

inline bws::domain::ConstBufferView toConstBufferView(const juce::AudioBuffer<float>& buf BW_LIFETIMEBOUND) noexcept
{
    return {buf.getArrayOfReadPointers(), buf.getNumChannels(), buf.getNumSamples()};
}

/**
 * Zero-copy reverse adapter: bws::domain::BufferView → juce::AudioBuffer<float>.
 *
 * Constructs a non-owning AudioBuffer pointing into the BufferView's channel array.
 * Valid as long as the source BufferView is alive - always true within processBlock scope.
 *
 * All juce::AudioBuffer methods work correctly on the returned non-owning buffer
 * (getWritePointer, applyGain, copyFrom, getMagnitude, etc.) EXCEPT setSize() and
 * makeCopyOf() which would attempt reallocation.
 *
 *
 */
inline juce::AudioBuffer<float> toJuceBufferView(const bws::domain::BufferView& bv) noexcept
{
    return juce::AudioBuffer<float>(bv.channels, bv.numChannels, bv.numSamples);
}

} // namespace bws::adapters
