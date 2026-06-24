// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cassert>
#include <cstdint>
#include "bw_audio_types/BufferView.h"

namespace bws::domain
{

/**
 * Read-only view over a multichannel audio buffer.
 *
 * Use this for all consumers that only read audio data:
 * visualizers, meters, analyzers, correlation meters, goniometers.
 *
 * Implicitly constructible from BufferView - the audio processor
 * holds a writable BufferView and passes ConstBufferView to read-only
 * consumers with no explicit cast required.
 *
 * RT-safe: no allocation, no virtual dispatch, trivially constructible.
 *
 * Carries the same lifetime-analysis contract as BufferView.
 */
struct BWS_AUDIO_TYPES_GSL_POINTER(float) ConstBufferView
{
    const float* const* channels = nullptr;
    int numChannels = 0;
    int numSamples = 0;

    // Default construction - invalid view
    ConstBufferView() = default;

    // Implicit conversion from BufferView - writable to read-only is always safe
    /*implicit*/ ConstBufferView(const BufferView& bv) noexcept
        : channels(bv.channels)
        , numChannels(bv.numChannels)
        , numSamples(bv.numSamples)
    {}

    // Direct construction from raw pointers
    ConstBufferView(const float* const* chs, int nCh, int nSamples) noexcept
        : channels(chs)
        , numChannels(nCh)
        , numSamples(nSamples)
    {}

    [[nodiscard]] bool isValid() const noexcept { return channels != nullptr && numChannels > 0 && numSamples > 0; }

    [[nodiscard]] const float* channel(int index) const noexcept
    {
        assert(index >= 0 && index < numChannels);
        return channels[index];
    }
};

} // namespace bws::domain
