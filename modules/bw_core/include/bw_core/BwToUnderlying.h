// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "bw_core/BwCompilerFeatures.h"
#include <type_traits>

#if BW_HAS_TO_UNDERLYING
#include <utility>
#endif

namespace bws::domain
{

/**
 * Type-safe enum-to-integer conversion.
 *
 * Uses std::to_underlying (C++23) when available, falls back to
 * static_cast<std::underlying_type_t<E>>(e) on C++20 targets.
 *
 * Usage:
 *   enum class Mode : int { A, B, C };
 *   int v = bws::domain::to_underlying(Mode::B);  // v == 1
 */
template <typename E>
[[nodiscard]] constexpr std::underlying_type_t<E> to_underlying(E e) noexcept
{
#if BW_HAS_TO_UNDERLYING
    return std::to_underlying(e);
#else
    return static_cast<std::underlying_type_t<E>>(e);
#endif
}

} // namespace bws::domain
