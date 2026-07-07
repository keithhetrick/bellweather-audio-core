// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

/// @file
/// @brief Scoped real-time guard that observes audio-thread code for allocations and locks.

#include <cstddef>
#include <atomic>
#include "bw_rt/AudioThreadScope.h"
#include "bw_rt/RtViolationBuffer.h"

namespace bws::rt
{

// Compile out the guard when an external runtime sanitizer owns observation.
// Running two interceptor mechanisms over the same audio-thread scope would
// pollute both diagnostics.
#if defined(BWS_RT_GUARD_DEFER_TO_RTSAN) && BWS_RT_GUARD_DEFER_TO_RTSAN
#undef BWS_ENABLE_RT_GUARD
#define BWS_ENABLE_RT_GUARD 0
#endif

#if defined(BWS_ENABLE_RT_GUARD) && BWS_ENABLE_RT_GUARD
constexpr bool kRtGuardEnabled = true;
#else
constexpr bool kRtGuardEnabled = false;
#endif

RtViolationBuffer& globalViolationBuffer() noexcept;
void recordViolation(ViolationType type, const char* message) noexcept;
void ensureOverridesLinked() noexcept;

//  containment observability - non-heap delete guard counters.
// Query these to verify the isHeapPointer guard is not silently hiding
// unrelated invalid-free defects.
uint64_t skippedNonHeapDeleteTotal() noexcept;
uint64_t skippedNonHeapDeleteUniqueCount() noexcept;

inline bool isAudioThread() noexcept
{
    return AudioThreadScope::isAudioThread();
}

class RtMutex
{
public:
    RtMutex() = default;
    RtMutex(const RtMutex&) = delete;
    RtMutex& operator=(const RtMutex&) = delete;

    void lock();
    void unlock();
    bool try_lock();

private:
    std::atomic_flag flag;
};

} // namespace bws::rt
