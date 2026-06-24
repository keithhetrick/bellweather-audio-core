// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_rt/RtGuard.h"
#include <new>

namespace bws::rt
{

RtViolationBuffer& globalViolationBuffer() noexcept
{
    ensureOverridesLinked();
    static RtViolationBuffer buffer;
    return buffer;
}

void recordViolation(ViolationType type, const char* message) noexcept
{
    globalViolationBuffer().record(type, message);
}

void RtMutex::lock()
{
    if constexpr (kRtGuardEnabled)
    {
        if (isAudioThread())
            recordViolation(ViolationType::Lock, "lock");
    }
    while (flag.test_and_set(std::memory_order_acquire))
    {}
}

bool RtMutex::try_lock()
{
    if constexpr (kRtGuardEnabled)
    {
        if (isAudioThread())
            recordViolation(ViolationType::Lock, "try_lock");
    }
    return !flag.test_and_set(std::memory_order_acquire);
}

void RtMutex::unlock()
{
    flag.clear(std::memory_order_release);
}

} // namespace bws::rt
