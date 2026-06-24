// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once

/// @file
/// @brief Result type for fallible operations - std::expected on C++23, a compatible struct fallback on C++20.

// =============================================================================
// BwResult.h - Permanent adapter layer for fallible port operations.
//
// On C++23 (BW_HAS_EXPECTED = 1): BwResult<T> IS std::expected<T, string_view>
// On C++20 (BW_HAS_EXPECTED = 0): BwResult<T> IS a compatible struct fallback
//
// Call sites are IDENTICAL on both machines.
// Monadic chaining (.and_then, .or_else, .transform) available on C++23 only.
//
// Migration note (once C++23 std::expected is the minimum supported path):
//   1. Delete this file entirely
//   2. Replace all BwResult<T> with std::expected<T, std::string_view>
//   3. Replace BwSuccess(v) with direct value construction
//   4. Replace BwFailure(msg) with std::unexpected(msg)
//   5. Replace BwSuccessVoid() with std::expected<void,string_view>{}
//   6. Replace BwFailureVoid(msg) with std::expected<void,string_view>(unexpected(msg))
//   7. Verification:
//      grep -rn "BwResult\|BwSuccess\|BwFailure\|BwSuccessVoid" modules/ plugins/
//      Expected: zero hits
// =============================================================================

#include <bw_core/BwCompilerFeatures.h>
#include <string_view>
#include <type_traits>
#include <utility>

#if BW_HAS_EXPECTED

// =============================================================================
// C++23 path - BwResult IS std::expected
// =============================================================================
#include <expected>

namespace bws::domain
{

template <typename T>
using BwResult = std::expected<T, std::string_view>;

template <typename T>
[[nodiscard]] constexpr BwResult<std::remove_cvref_t<T>> BwSuccess(T&& val) noexcept(
    std::is_nothrow_constructible_v<std::remove_cvref_t<T>, T&&>)
{
    return BwResult<std::remove_cvref_t<T>>(std::forward<T>(val));
}

template <typename T>
[[nodiscard]] constexpr BwResult<T> BwFailure(std::string_view msg) noexcept
{
    return BwResult<T>(std::unexpected(msg));
}

[[nodiscard]] inline BwResult<void> BwSuccessVoid() noexcept
{
    return BwResult<void>();
}

[[nodiscard]] inline BwResult<void> BwFailureVoid(std::string_view msg) noexcept
{
    return BwResult<void>(std::unexpected(msg));
}

} // namespace bws::domain

#else

// =============================================================================
// C++20 fallback - struct with std::expected-compatible API surface.
// Mirrors std::expected so removal is mechanical.
// Fields are public (matching original BwResult design).
// =============================================================================

namespace bws::domain
{

template <typename T>
struct BwResult
{
    // --- std::expected-compatible API ---
    [[nodiscard]] bool has_value() const noexcept { return ok_; }
    [[nodiscard]] explicit operator bool() const noexcept { return ok_; }

    [[nodiscard]] T& value() & { return value_; }
    [[nodiscard]] const T& value() const& { return value_; }
    [[nodiscard]] T&& value() && { return std::move(value_); }

    [[nodiscard]] T& operator*() & { return value_; }
    [[nodiscard]] const T& operator*() const& { return value_; }
    [[nodiscard]] T&& operator*() && { return std::move(value_); }

    [[nodiscard]] T* operator->() noexcept { return &value_; }
    [[nodiscard]] const T* operator->() const noexcept { return &value_; }

    [[nodiscard]] std::string_view error() const noexcept { return error_; }

    // --- Data (public for aggregate-style construction in factory functions) ---
    bool ok_ = true; // matches std::expected default (value state)
    T value_ {};
    std::string_view error_ {};
};

template <>
struct BwResult<void>
{
    [[nodiscard]] bool has_value() const noexcept { return ok_; }
    [[nodiscard]] explicit operator bool() const noexcept { return ok_; }
    [[nodiscard]] std::string_view error() const noexcept { return error_; }

    bool ok_ = true; // matches std::expected default (value state)
    std::string_view error_ {};
};

// --- Factory functions (identical call syntax on both machines) ---

template <typename T>
[[nodiscard]] inline BwResult<std::remove_cvref_t<T>> BwSuccess(T&& val) noexcept(
    std::is_nothrow_constructible_v<std::remove_cvref_t<T>, T&&>)
{
    BwResult<std::remove_cvref_t<T>> r;
    r.ok_ = true;
    r.value_ = std::forward<T>(val);
    return r;
}

template <typename T>
[[nodiscard]] inline BwResult<T> BwFailure(std::string_view msg) noexcept
{
    BwResult<T> r;
    r.ok_ = false;
    r.error_ = msg;
    return r;
}

[[nodiscard]] inline BwResult<void> BwSuccessVoid() noexcept
{
    BwResult<void> r;
    r.ok_ = true;
    return r;
}

[[nodiscard]] inline BwResult<void> BwFailureVoid(std::string_view msg) noexcept
{
    BwResult<void> r;
    r.ok_ = false;
    r.error_ = msg;
    return r;
}

} // namespace bws::domain

#endif
