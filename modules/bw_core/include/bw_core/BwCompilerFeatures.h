// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once

/// @file
/// @brief Cross-toolchain C++ feature detection and portable capability macros (BW_HAS_*, BW_ALWAYS_INLINE,
/// BW_LIFETIMEBOUND).

// =============================================================================
// BwCompilerFeatures.h
// Cross-toolchain C++ feature detection for Bellweather libraries.
//
// Probe data sources:
//   Old machine: Apple Clang 14.0.0, macOS 12 Monterey, x86_64
//   New machine: Apple Clang 21.0.0, macOS 26 Tahoe, arm64
//   Deployment target: macOS 11.0 (CMakeLists.txt:6-7)
//
// Gate tiers:
// compile-time only: no runtime dep, safe on all targets
// header-only stdlib: __cpp_* macro gate only
// dylib runtime dep: __cpp_* macro + deployment target gate
// modules: CMake gate only (BW_HAS_MODULES set by CMake)
//
// Rules:
//   - Never add version number checks (__clang_major__, _MSC_VER etc.)
//   - Always use __cpp_* macros or __has_include() - never vendor guards
//   - Apple libc++ macro gap: __cpp_lib_span and __cpp_lib_ranges read 0
//     on Clang 14 even when headers exist - use __has_include() fallback
//   - To add a new feature: add probe data as a comment, then add gate
//
// The convenience macros below (BW_LIKELY, BW_ASSUME, BW_VECTORIZE_LOOP,
// BW_CONSTEVAL, ...) are a provided portability vocabulary for consumers; not
// all are used in-tree.
//
// Compatibility note:
//   BW_HAS_CONSTEVAL = 1 in C++20 mode on Apple Clang 21 (reports 202211
//   even under -std=c++20). Older Apple Clang versions may report 0. The
//   C++20 smoke test validates gate logic, not every historical compiler value.
//
// MSVC note:
//   /Zc:__cplusplus must be set for __cplusplus to report the real standard
//   value on MSVC. This is configured in cmake/BwsWarnings.cmake.
// =============================================================================

#include <version>

// -----------------------------------------------------------------------------
// TIER 1 - Compile-time language features
// No runtime dependency. Safe unconditionally on all deployment targets.
// These are documented here for inventory purposes - no #if guard needed.
//
// Available on NEW machine (Clang 21, -std=c++23):
//   concepts, requires, constinit, [[likely]], [[unlikely]],
//   three-way comparison (spaceship), designated initializers
//
// Available on OLD machine (Clang 14, -std=c++20):
//   same list - all C++20 language features, Clang 14 supports them
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// TIER 1 - BW_VECTORIZE_LOOP
// Permit loop vectorization where strict FP ordering would block it.
// Clang-only; no-op on GCC / MSVC. Verified: Apple Clang 21.0.0 on arm64.
//
// REQUIRED usage pattern - place AT THE START of a fresh { ... } block
// containing only the target loop:
//
//   {
//       BW_VECTORIZE_LOOP
//       for (int i = 0; i < n; ++i)
//           sum += data[i] * data[i];
//   }
//   // fp reassociate(on) scope ends here - does NOT leak past the brace
//
// Rationale:
// - `#pragma clang fp reassociate(on)` MUST be at the start of a compound
//   statement (clang hard rule). Placing it inside a pre-existing block
//   after other statements emits a compile error.
// - Pragma order matters: `fp reassociate(on)` must precede
//   `loop vectorize(enable)` in the macro expansion. The reverse order
//   also produces a compile error.
// - Scope ends at the enclosing `}`, so the block wrapper prevents relaxed
//   FP semantics from leaking into code below the target loop.
//
// For FP reductions, prefer `std::fmax` / `std::fmin` over `std::max` /
// `std::min`. The `f*` variants are IEEE-754 deterministic and the
// vectorizer recognizes them as reductions under strict FP; the generic
// `std::max` is not recognized and forces Force=true warnings.
//
// Do NOT use this macro to silence legitimate reductions that indicate a
// real algorithmic problem. Reserved for cases where a pure numerical loop
// is provably reorderable.
// -----------------------------------------------------------------------------
#if defined(__clang__)
#define BWS_PRAGMA(x) _Pragma(#x)
// ORDER MATTERS: fp reassociate FIRST (block-start requirement),
// loop vectorize SECOND (loop-attached). Reversed order errors.
#define BW_VECTORIZE_LOOP                \
    BWS_PRAGMA(clang fp reassociate(on)) \
    BWS_PRAGMA(clang loop vectorize(enable))
#else
#define BW_VECTORIZE_LOOP
#endif

// [[likely]] / [[unlikely]] branch hints (C++20).
#define BW_LIKELY   [[likely]]
#define BW_UNLIKELY [[unlikely]]

// Force-inline hint, portable across GCC/Clang and MSVC (avoids C5030).
#if defined(__GNUC__) || defined(__clang__)
#define BW_ALWAYS_INLINE [[gnu::always_inline]]
#elif defined(_MSC_VER)
#define BW_ALWAYS_INLINE [[msvc::forceinline]]
#else
#define BW_ALWAYS_INLINE
#endif

// Lifetime-bound parameter hint. Clang / MSVC spellings; empty elsewhere.
#if __has_cpp_attribute(clang::lifetimebound)
#define BW_LIFETIMEBOUND [[clang::lifetimebound]]
#elif __has_cpp_attribute(msvc::lifetimebound)
#define BW_LIFETIMEBOUND [[msvc::lifetimebound]]
#else
#define BW_LIFETIMEBOUND
#endif

// BW_ASSUME(expr): [[assume]] (C++23) with builtin fallbacks. UB if expr is
// false; expr must be side-effect free (it is not evaluated).
#if __has_cpp_attribute(assume) >= 202207L
#define BW_ASSUME(expr) [[assume(expr)]]
#elif defined(__clang__)
#define BW_ASSUME(expr) __builtin_assume(expr)
#elif defined(__GNUC__)
#define BW_ASSUME(expr)              \
    do                               \
    {                                \
        if (!(expr))                 \
            __builtin_unreachable(); \
    } while (0)
#elif defined(_MSC_VER)
#define BW_ASSUME(expr) __assume(expr)
#else
#define BW_ASSUME(expr) ((void)0)
#endif

// -----------------------------------------------------------------------------
// TIER 1 - consteval
// Probe: OLD=0 (genuinely absent), NEW=202211 (c++20 mode), 202211 (c++23)
// Compile-time only. No deployment target gate needed.
// -----------------------------------------------------------------------------
#if __cpp_consteval >= 202211L
#define BW_HAS_CONSTEVAL 1
#else
#define BW_HAS_CONSTEVAL 0
#endif

// Compatibility shim - use BW_CONSTEVAL in place of consteval.
// Degrades to constexpr on compilers without consteval support.
//
// WARNING: consteval and constexpr have different semantics.
// consteval MUST be evaluated at compile time; constexpr CAN run at runtime.
// BW_CONSTEVAL does NOT guarantee compile-time evaluation on all targets.
// If compile-time-only evaluation is a correctness requirement (not just an
// optimization), use static_assert or if consteval guards at the call site
// rather than relying on BW_CONSTEVAL alone.
#if BW_HAS_CONSTEVAL
#define BW_CONSTEVAL consteval
#else
#define BW_CONSTEVAL constexpr
#endif

// -----------------------------------------------------------------------------
// TIER 1 - if consteval (C++23)
// Probe: OLD=UNDEFINED, NEW c++20=UNDEFINED, NEW c++23=202106
// Compile-time branching without SFINAE. Replaces is_constant_evaluated().
// -----------------------------------------------------------------------------
#if __cpp_if_consteval >= 202106L
#define BW_HAS_IF_CONSTEVAL 1
#else
#define BW_HAS_IF_CONSTEVAL 0
#endif

// -----------------------------------------------------------------------------
// TIER 1 - Deducing this (C++23)
// Probe: OLD=UNDEFINED, NEW c++20=UNDEFINED, NEW c++23=202110
// Eliminates CRTP boilerplate in DSP chain patterns.
// -----------------------------------------------------------------------------
#if __cpp_explicit_this_parameter >= 202110L
#define BW_HAS_DEDUCING_THIS 1
#else
#define BW_HAS_DEDUCING_THIS 0
#endif

// -----------------------------------------------------------------------------
// TIER 2 - std::span
// Probe: OLD __cpp_lib_span=0 (Apple macro gap - header exists, macro absent)
//        NEW __cpp_lib_span=202002 (c++20 + c++23 mode)
// Apple Clang 14 ships <span> but doesn't set __cpp_lib_span.
// Gate on __has_include to avoid false negative on old machine.
// Header-only in libc++. Safe on macOS 11.0 deployment target.
//
// Note: std::span is already used unconditionally in bw_audio_types
// (BwStateBlob.h). BW_HAS_SPAN is guaranteed 1 on all supported
// configurations. This gate exists for completeness and external consumers.
// -----------------------------------------------------------------------------
#if __has_include(<span>) && __cpp_lib_span >= 202002L
#define BW_HAS_SPAN 1
#elif __has_include(<span>)
// Apple Clang 14 macro gap - header present, macro absent
#define BW_HAS_SPAN 1
#else
#define BW_HAS_SPAN 0
#endif

// -----------------------------------------------------------------------------
// TIER 2 - std::ranges
// Probe: OLD __cpp_lib_ranges=0 (Apple macro gap - same pattern as span)
//        NEW c++20=202110, NEW c++23=202406 (substantial upgrade)
// Gate on __has_include to avoid false negative on old machine.
// Note: std::views::zip ships but __cpp_lib_ranges_zip=UNDEFINED on both
// machines. Use compile test via BW_HAS_RANGES_ZIP below.
// -----------------------------------------------------------------------------
#if __has_include(<ranges>) && __cpp_lib_ranges >= 202110L
#define BW_HAS_RANGES 1
#elif __has_include(<ranges>)
// Apple Clang 14 macro gap
#define BW_HAS_RANGES 1
#else
#define BW_HAS_RANGES 0
#endif

// std::views::zip - macro UNDEFINED on both machines despite header shipping.
// __cpp_lib_ranges_zip is UNDEFINED even in C++23 mode on Apple Clang 21,
// despite std::views::zip compiling successfully. Gate on __cplusplus as
// the only viable detection path.
#if BW_HAS_RANGES && __cplusplus >= 202302L
#define BW_HAS_RANGES_ZIP 1
#else
#define BW_HAS_RANGES_ZIP 0
#endif

// -----------------------------------------------------------------------------
// TIER 2 - std::atomic_ref
// Probe: OLD=0, NEW=201806 (both c++20 and c++23 mode)
// Header-only. Safe on macOS 11.0 deployment target.
// RT-safe shared state without locks - directly serves RT-safety rules.
// -----------------------------------------------------------------------------
#if __cpp_lib_atomic_ref >= 201806L
#define BW_HAS_ATOMIC_REF 1
#else
#define BW_HAS_ATOMIC_REF 0
#endif

// -----------------------------------------------------------------------------
// TIER 2 - std::bit_cast
// Probe: OLD=0, NEW=201806 (both c++20 and c++23 mode)
// Header-only. Safe on macOS 11.0 deployment target.
// Type-safe DSP bit manipulation (replaces reinterpret_cast patterns).
// -----------------------------------------------------------------------------
#if __cpp_lib_bit_cast >= 201806L
#define BW_HAS_BIT_CAST 1
#else
#define BW_HAS_BIT_CAST 0
#endif

// -----------------------------------------------------------------------------
// TIER 2 - std::expected<T,E> (C++23)
// Probe: OLD=UNDEFINED, NEW c++20=UNDEFINED, NEW c++23=202211
// Header-only in libc++. Safe on macOS 11.0 deployment target.
// Replaces BwResult<T> - strongest C++23 signal in the codebase.
// Override-guarded (cf. BW_HAS_MODULES): -DBW_HAS_EXPECTED=0 forces the C++20
// fallback path so it can be built/tested independently of the toolchain.
// -----------------------------------------------------------------------------
#ifndef BW_HAS_EXPECTED
#if __cpp_lib_expected >= 202211L
#define BW_HAS_EXPECTED 1
#else
#define BW_HAS_EXPECTED 0
#endif
#endif

// -----------------------------------------------------------------------------
// TIER 2 - std::flat_map (C++23)
// Probe: OLD=UNDEFINED, NEW c++20=UNDEFINED, NEW c++23=202207
// Header-only. Safe on macOS 11.0 deployment target.
// Cache-friendly ordered map for DSP parameter lookups.
// -----------------------------------------------------------------------------
#if __cpp_lib_flat_map >= 202207L
#define BW_HAS_FLAT_MAP 1
#else
#define BW_HAS_FLAT_MAP 0
#endif

// -----------------------------------------------------------------------------
// TIER 2 - std::mdspan (C++23)
// Probe: OLD=UNDEFINED, NEW c++20=UNDEFINED, NEW c++23=202207
// Header-only. Safe on macOS 11.0 deployment target.
// Long-term candidate for multi-dimensional audio views.
// Not a BufferView replacement today - capability probe only.
// -----------------------------------------------------------------------------
#if __cpp_lib_mdspan >= 202207L
#define BW_HAS_MDSPAN 1
#else
#define BW_HAS_MDSPAN 0
#endif

// -----------------------------------------------------------------------------
// TIER 2 - std::source_location (C++20)
// Probe: NEW c++20=201907, NEW c++23=201907
// Header-only. Safe on macOS 11.0 deployment target.
// Replaces __FILE__/__LINE__ macros with structured call-site info.
// -----------------------------------------------------------------------------
#if __cpp_lib_source_location >= 201907L
#define BW_HAS_SOURCE_LOCATION 1
#else
#define BW_HAS_SOURCE_LOCATION 0
#endif

// -----------------------------------------------------------------------------
// TIER 2 - std::to_underlying (C++23)
// Probe: NEW c++20=UNDEFINED, NEW c++23=202102
// Header-only. Safe on macOS 11.0 deployment target.
// Type-safe enum-to-integer conversion, replaces static_cast<int>(e).
// Fallback utility provided in bw_core/BwToUnderlying.h.
// -----------------------------------------------------------------------------
#if __cpp_lib_to_underlying >= 202102L
#define BW_HAS_TO_UNDERLYING 1
#else
#define BW_HAS_TO_UNDERLYING 0
#endif

// -----------------------------------------------------------------------------
// TIER 3 - std::print / std::println (C++23)
// Probe: OLD=UNDEFINED, NEW c++20=UNDEFINED, NEW c++23=202207
// DYLIB RISK: locale machinery has runtime dylib symbols.
// Gate: compiler macro AND deployment target >= 13.0 to be safe.
// Low priority for audio plugins - not recommended for use in DSP paths.
//
// Note: Always 0 at the current macOS 11.0 deployment target.
// Activates only if deployment target is bumped to >= 13.0.
// -----------------------------------------------------------------------------
#if __cpp_lib_print >= 202207L && defined(__APPLE__) && defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__) && \
    __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ >= 130000
#define BW_HAS_PRINT 1
#else
#define BW_HAS_PRINT 0
#endif

// -----------------------------------------------------------------------------
// TIER 4 - C++20 Named Modules
// __cpp_modules = UNDEFINED on BOTH machines (Apple doesn't advertise this).
// Gate is set by CMake via check_source_compiles - not detectable here.
// Default to 0; CMake overrides to 1 when module compilation is verified.
// See: cmake/BwModuleSupport.cmake.
// -----------------------------------------------------------------------------
#ifndef BW_HAS_MODULES
#define BW_HAS_MODULES 0
#endif

// -----------------------------------------------------------------------------
// Convenience: C++ standard version shorthands
// -----------------------------------------------------------------------------
#if __cplusplus >= 202302L
#define BW_CPP23 1
#else
#define BW_CPP23 0
#endif

#if __cplusplus >= 202002L
#define BW_CPP20 1
#else
#define BW_CPP20 0
#endif

// -----------------------------------------------------------------------------
// Compile-time summary assert - fails fast if standard is below C++20
// -----------------------------------------------------------------------------
static_assert(__cplusplus >= 202002L, "Bellweather requires C++20 or later. "
                                      "Set CMAKE_CXX_STANDARD to at least 20 in CMakeLists.txt.");
