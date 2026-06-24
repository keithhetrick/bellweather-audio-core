// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

/**
 * @file AllocCounter.h
 * @brief Counting allocator for audio-thread no-allocation regression tests
 *
 * Shared by no-allocation tests that need per-thread allocation counts.
 * Include this header when a test owns the assertion and
 * AllocCounter_GlobalOperators.h when that translation unit also owns the
 * global operator new/delete hooks.
 *
 * USAGE:
 *   #include "bws/test/AllocCounter.h"
 *   ...
 *   TEST_CASE("...does not allocate...")
 *   {
 *       // ... pre-warm any first-time allocation paths ...
 *       AllocCounterScope scope;
 *       // ... operation under test ...
 *       REQUIRE(scope.delta() == 0);
 *   }
 *
 * REQUIREMENTS: the consuming TU must define global operator new / delete
 * exactly once across the linked binary. See AllocCounter_GlobalOperators.h
 * for the canonical inline-define pattern (single-TU expansion).
 *
 * Native-first: replaces global new/delete via per-thread counters; no 3rd-party
 * allocation-tracker library.
 */

#include <cstddef>

namespace bws::test
{

struct AllocCounter
{
    static thread_local long long count;
    static thread_local bool active;
};

inline thread_local long long AllocCounter::count = 0;
inline thread_local bool AllocCounter::active = false;

struct AllocCounterScope
{
    long long startCount;

    AllocCounterScope() noexcept
        : startCount(AllocCounter::count)
    {
        AllocCounter::active = true;
    }

    ~AllocCounterScope() noexcept { AllocCounter::active = false; }

    long long delta() const noexcept { return AllocCounter::count - startCount; }
};

} // namespace bws::test
