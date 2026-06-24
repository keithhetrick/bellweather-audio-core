// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cmath>
#include <cstddef>

// Self-contained (bw_rt is dependency-free); mirrors bw_core's BW_ALWAYS_INLINE.
#ifndef BW_ALWAYS_INLINE
#if defined(__GNUC__) || defined(__clang__)
#define BW_ALWAYS_INLINE [[gnu::always_inline]]
#elif defined(_MSC_VER)
#define BW_ALWAYS_INLINE [[msvc::forceinline]]
#else
#define BW_ALWAYS_INLINE
#endif
#endif

namespace bws::rt
{

/// Snap-to-zero helper for non-arithmetic float paths.
///
/// FPCR.FZ (ARM64) and MXCSR.FZ+DAZ (x86) flush arithmetic results only -
/// not memcpy / std::copy / register loads. ARM AArch64 pre-v8.7 has no
/// DAZ equivalent (Apple Silicon M1/M2/M3 are v8.5/v8.6, do not honor
/// v8.7 FPCR.FIZ). Manual snap-to-zero at boundaries - filter return
/// values, buffer copies, EMA outputs decaying toward zero - is the only
/// portable defense.
///
/// Threshold matches TptOnePole's kSubnormalFlush (1e-20f) - well above
/// smallest normal float32 (~1.18e-38). Magnitudes below are inaudible
/// (-400 dBFS) numerical residue.
///
/// RT-safe: no allocation, no locks.

inline constexpr float kSnapThreshold = 1e-20f;

BW_ALWAYS_INLINE inline float snapToZero(float x) noexcept
{
    return (std::fabs(x) < kSnapThreshold) ? 0.0f : x;
}

inline void snapToZero(float* data, std::size_t count) noexcept
{
    for (std::size_t i = 0; i < count; ++i)
    {
        data[i] = snapToZero(data[i]);
    }
}

} // namespace bws::rt
