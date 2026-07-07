// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

/// @file
/// @brief Compile-time real-time-safety annotations (BWS_NONBLOCKING / BWS_NONALLOCATING) for audio-thread code.

#ifndef __has_warning
#define __has_warning(x) 0
#endif

// =============================================================================
// RtSafety.h - compile-time RT-safety annotations for audio-thread code.
//
// These macros expose Clang's function-effect attributes when the compiler
// supports them and collapse to no-ops elsewhere.
//
// Usage:
//
//   // 1. Annotate audio-thread entry-points and primitives.
//   //    Attribute placement: AFTER noexcept (C++ grammar). The order
//   //    `f() BWS_NONBLOCKING noexcept` does NOT parse - the attribute-
//   //    specifier-seq comes after the noexcept-specifier per [dcl.fct].
//   void processBlock(...) noexcept BWS_NONBLOCKING;
//
//   // 2. Single-statement suppression at a known-safe vendor callsite.
//   [[clang::suppress(function-effects)]] vendorCall();
//
//   // 3. Multi-statement block suppression for legitimately-blocking init.
//   BWS_NONBLOCKING_UNSAFE(
//       BlockingInitializer initializer;
//       initializer.run();
//   )
//
// Both suppression forms locally silence -Wfunction-effects WITHOUT
// weakening the enclosing function's [[clang::nonblocking]] attribute.
//
// This header is Layer 1 only (compile-time / FEA). Layer 2 runtime guard
// lives at modules/bw_rt/include/bw_rt/RtGuard.h. Layer 3 (RTSan) shares
// the same [[clang::nonblocking]] attribute - zero double-annotation tax.
// =============================================================================

#if defined(__clang__) && defined(__has_cpp_attribute) && defined(__has_warning)
#if __has_cpp_attribute(clang::nonblocking) && __has_warning("-Wfunction-effects")
#define BWS_HAS_FEA 1
#else
#define BWS_HAS_FEA 0
#endif
#else
#define BWS_HAS_FEA 0
#endif

#if BWS_HAS_FEA
#define BWS_NONBLOCKING   [[clang::nonblocking]]
#define BWS_NONALLOCATING [[clang::nonallocating]]
#else
#define BWS_NONBLOCKING
#define BWS_NONALLOCATING
#endif

#if defined(__clang__) && defined(__has_warning) && __has_warning("-Wfunction-effects")
#define BWS_NONBLOCKING_UNSAFE(...)                                                             \
    _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wfunction-effects\"") \
        __VA_ARGS__ _Pragma("clang diagnostic pop")
#else
#define BWS_NONBLOCKING_UNSAFE(...) __VA_ARGS__
#endif
