// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

/**
 * @file AllocCounter_GlobalOperators.h
 * @brief Single-TU expansion of global new/delete overrides for AllocCounter
 *
 * Include this header in EXACTLY ONE translation unit per test binary.
 * It defines global ::operator new / ::operator delete that increment
 * bws::test::AllocCounter::count when bws::test::AllocCounter::active.
 * ThreadSanitizer builds do not install these overrides because the TSan C++
 * runtime owns the same allocation/deallocation interceptor symbols.
 *
 * Splitting from AllocCounter.h prevents ODR violations: the data members
 * and AllocCounterScope live in AllocCounter.h (safe to include in many
 * TUs); the global-operator definitions live here (must be in exactly one
 * TU per binary).
 */

#include "bws/test/AllocCounter.h"

#include <cstdlib>
#include <new>

#ifndef __has_feature
#define __has_feature(x) 0
#endif

#if !defined(__SANITIZE_THREAD__) && !__has_feature(thread_sanitizer)
void* operator new(std::size_t n)
{
    if (bws::test::AllocCounter::active)
        ++bws::test::AllocCounter::count;
    if (auto* p = std::malloc(n))
        return p;
    throw std::bad_alloc {};
}

void operator delete(void* p) noexcept
{
    std::free(p);
}

void operator delete(void* p, std::size_t) noexcept
{
    std::free(p);
}
#endif
