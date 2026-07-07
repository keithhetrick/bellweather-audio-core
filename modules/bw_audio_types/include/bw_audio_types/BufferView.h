// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cassert>
#include <cstdint>

#if defined(__clang__)
#define BWS_AUDIO_TYPES_GSL_POINTER(T) [[gsl::Pointer(T)]]
#define BWS_AUDIO_TYPES_GSL_OWNER(T)   [[gsl::Owner(T)]]
#else
#define BWS_AUDIO_TYPES_GSL_POINTER(T)
#define BWS_AUDIO_TYPES_GSL_OWNER(T)
#endif

namespace bws::domain
{

/**
 * Non-owning view over a multichannel audio buffer.
 * Non-owning multichannel float audio view.
 * RT-safe: no allocation, no virtual dispatch, trivially constructible.
 *
 * Clang consumes the GSL lifetime annotation for static analysis. Other
 * compilers see the same public type without warning on an ignored attribute.
 */
struct BWS_AUDIO_TYPES_GSL_POINTER(float) BufferView
{
    float* const* channels = nullptr;
    int numChannels = 0;
    int numSamples = 0;

    [[nodiscard]] bool isValid() const noexcept { return channels != nullptr && numChannels > 0 && numSamples > 0; }

    [[nodiscard]] float* channel(int index) const noexcept
    {
        assert(index >= 0 && index < numChannels);
        return channels[index];
    }
};

} // namespace bws::domain
