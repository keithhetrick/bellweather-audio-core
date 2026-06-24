// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
// Contract source: modules/bw_rt/include/bw_rt/RtGuard.h
// RT-guard containment observability regression tests.
//
// The ValueTreeRetainer tests that used to live here moved to
// modules/bw_juce_adapters/tests/test_value_tree_retainer.cpp when
// ValueTreeRetainer.h was buried into bw_juce_adapters/src/. bw_rt is
// JUCE-clean; tests for JUCE-tied implementations belong with their module.

#include <catch2/catch_test_macros.hpp>
#include <cstdlib>
#include <cstdint>
#include "bw_rt/RtGuard.h"

#ifndef __has_feature
#define __has_feature(x) 0
#endif

TEST_CASE("Heap delete does not increment skip counter", "[rt_guard][bug023]")
{
    if constexpr (!bws::rt::kRtGuardEnabled)
    {
        SUCCEED("RT guard delete overrides disabled for this instrumentation lane");
        return;
    }

    const uint64_t before = bws::rt::skippedNonHeapDeleteTotal();
    auto* heapPtr = new int(42);
    delete heapPtr;
    REQUIRE(bws::rt::skippedNonHeapDeleteTotal() == before);
}

// The non-heap delete guard is Apple-only: isHeapPointer probes with malloc_size,
// which has no portable equivalent. Elsewhere the override degrades to a plain
// free, so deleting a non-heap pointer is undefined and the counters stay zero.
#if defined(__APPLE__)
TEST_CASE("Non-heap delete guard increments counters", "[rt_guard][bug023]")
{
    if constexpr (!bws::rt::kRtGuardEnabled)
    {
        SUCCEED("RT guard delete overrides disabled for this instrumentation lane");
        return;
    }

#if !defined(NDEBUG) || defined(__SANITIZE_ADDRESS__) || __has_feature(address_sanitizer)
    SUCCEED("Non-heap delete containment asserts in debug/ASan builds");
    return;
#endif

    alignas(16) char stackBuffer[64] = {};
    void* stackPtr = static_cast<void*>(stackBuffer);

    const uint64_t beforeTotal = bws::rt::skippedNonHeapDeleteTotal();
    const uint64_t beforeUnique = bws::rt::skippedNonHeapDeleteUniqueCount();

    SECTION("total increments on non-heap delete")
    {
        ::operator delete(stackPtr);
        REQUIRE(bws::rt::skippedNonHeapDeleteTotal() > beforeTotal);
    }

    SECTION("unique count does not decrease")
    {
        ::operator delete(stackPtr);
        REQUIRE(bws::rt::skippedNonHeapDeleteUniqueCount() >= beforeUnique);
    }

    SECTION("second delete of same address increments total but not unique")
    {
        ::operator delete(stackPtr);
        const uint64_t afterTotal1 = bws::rt::skippedNonHeapDeleteTotal();
        const uint64_t afterUnique1 = bws::rt::skippedNonHeapDeleteUniqueCount();

        ::operator delete(stackPtr);
        REQUIRE(bws::rt::skippedNonHeapDeleteTotal() > afterTotal1);
        REQUIRE(bws::rt::skippedNonHeapDeleteUniqueCount() == afterUnique1);
    }
}
#endif
