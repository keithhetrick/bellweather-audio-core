// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_rt/RtGuard.h"
#include <cassert>
#include <cstdlib>
#include <new>
#include <atomic>

#if defined(__APPLE__)
#include <malloc/malloc.h>
#endif

#ifndef __has_feature
#define __has_feature(x) 0
#endif

namespace
{
thread_local bool rtGuardReentry = false;

//==============================================================================
// Non-heap delete observability (containment tracking)
//
// Two counters track how often the isHeapPointer guard prevents a bad-free:
//   total   - every skipped free (may fire many times for the same address)
//   unique  - approximate distinct addresses (hash-sampled, 64 slots)
//
// In Debug/ASan builds: jassertfalse on first occurrence per unique address.
// In Release builds: no assert, no I/O, just atomic counters.
//==============================================================================
std::atomic<uint64_t> sSkippedNonHeapDeleteTotal {0};
std::atomic<uint64_t> sSkippedNonHeapDeleteUnique {0};

// Simple hash-sampled unique address tracker (lock-free, bounded memory).
// 64 slots - collisions just mean we under-count uniques slightly.
constexpr size_t kUniqueSlots = 64;
std::atomic<uintptr_t> seenAddresses[kUniqueSlots] = {};

inline void recordSkippedNonHeapDelete(void* ptr) noexcept
{
    sSkippedNonHeapDeleteTotal.fetch_add(1, std::memory_order_relaxed);

    const auto addr = reinterpret_cast<uintptr_t>(ptr);
    const size_t slot = (addr >> 3) % kUniqueSlots;

    uintptr_t expected = 0;
    if (seenAddresses[slot].compare_exchange_strong(expected, addr, std::memory_order_relaxed))
    {
        // First time seeing this slot - count as unique
        sSkippedNonHeapDeleteUnique.fetch_add(1, std::memory_order_relaxed);

#if !defined(NDEBUG) || defined(__SANITIZE_ADDRESS__) || __has_feature(address_sanitizer)
        // Debug/ASan: alert on first unique non-heap delete.
        // Breakpoint here to inspect which pointer is being guarded.
        assert(false && "Non-heap delete detected");
#endif
    }
}

// Returns true if ptr was returned by malloc/new and is safe to free.
// Prevents crashes when JUCE's StringHolderUtils::release() reaches
// operator delete[] with a pointer to constexpr static data (emptyString)
// due to cross-module isEmptyString() mismatches in the VST3 bundle.
inline bool isHeapPointer([[maybe_unused]] void* ptr) noexcept
{
#if defined(__APPLE__)
    return malloc_size(ptr) > 0;
#else
    // No portable heap-validity probe exists off Apple, so the guard degrades
    // to a plain free. The mismatch it protects against is an Apple-only VST3
    // bundling artifact, so it cannot occur here; the skipped-delete counters
    // are therefore always zero on non-Apple platforms.
    return true;
#endif
}
} // namespace

namespace bws::rt
{
void ensureOverridesLinked() noexcept {}

uint64_t skippedNonHeapDeleteTotal() noexcept
{
    return ::sSkippedNonHeapDeleteTotal.load(std::memory_order_relaxed);
}

uint64_t skippedNonHeapDeleteUniqueCount() noexcept
{
    return ::sSkippedNonHeapDeleteUnique.load(std::memory_order_relaxed);
}
} // namespace bws::rt

#if defined(__GNUC__)
#define BWS_RT_VIS_DEFAULT __attribute__((visibility("default")))
#else
#define BWS_RT_VIS_DEFAULT
#endif

BWS_RT_VIS_DEFAULT void* operator new(std::size_t size) noexcept(false)
{
    if constexpr (bws::rt::kRtGuardEnabled)
    {
        if (bws::rt::isAudioThread() && !rtGuardReentry)
        {
            rtGuardReentry = true;
            bws::rt::recordViolation(bws::rt::ViolationType::Alloc, "operator new");
            rtGuardReentry = false;
        }
    }
    if (void* ptr = std::malloc(size))
        return ptr;
    throw std::bad_alloc();
}

BWS_RT_VIS_DEFAULT void* operator new[](std::size_t size) noexcept(false)
{
    return ::operator new(size);
}

BWS_RT_VIS_DEFAULT void operator delete(void* ptr) noexcept
{
    if (ptr == nullptr)
        return;
    if (isHeapPointer(ptr))
        std::free(ptr);
    else
        recordSkippedNonHeapDelete(ptr);
}

BWS_RT_VIS_DEFAULT void operator delete[](void* ptr) noexcept
{
    ::operator delete(ptr);
}

BWS_RT_VIS_DEFAULT void operator delete(void* ptr, std::size_t) noexcept
{
    if (ptr == nullptr)
        return;
    if (isHeapPointer(ptr))
        std::free(ptr);
    else
        recordSkippedNonHeapDelete(ptr);
}

BWS_RT_VIS_DEFAULT void operator delete[](void* ptr, std::size_t) noexcept
{
    ::operator delete(ptr);
}

#undef BWS_RT_VIS_DEFAULT
