// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cassert>
#include <cstdint>
#include <cstddef>

namespace bws::domain
{

/** Maximum parameter count across Bellweather plugins.
 *  Sized to the largest current plugin with generous headroom.
 *  If a plugin exceeds this, change the constant and recompile. */
static constexpr size_t kMaxParams = 128;

using ParamId = uint32_t;

/**
 * Point-in-time snapshot of plain (denormalized) parameter values.
 * Values are in their natural DSP range - dB, ms, Hz, integer indices -
 * exactly as returned by APVTS getRawParameterValue() and as consumed
 * by DSP code. NOT normalized to [0,1].
 *
 * Plain values throughout, avoiding a normalization/denormalization
 * round-trip at the seam.
 *
 * Trivially copyable. Stack allocated. RT-safe by construction.
 */
struct ParamSnapshot
{
    float values[kMaxParams] {};
    uint32_t count = 0;

    [[nodiscard]] float get(ParamId id) const noexcept { return (id < count) ? values[id] : 0.0f; }

    void set(ParamId id, float normalizedValue) noexcept
    {
        assert(id < kMaxParams);
        if (id < kMaxParams)
        {
            values[id] = normalizedValue;
            if (id >= count)
                count = id + 1;
        }
    }
};

} // namespace bws::domain
