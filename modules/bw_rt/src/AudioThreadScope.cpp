// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_rt/AudioThreadScope.h"

#include <array>
#include <cstdint>
#include <type_traits>

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace bws::rt
{

namespace
{

constexpr std::size_t kAudioThreadSlotCount = 64;

struct AudioThreadSlot
{
    std::atomic<std::uintptr_t> token {};
    std::atomic<std::uint32_t> depth {};
};

static_assert(std::atomic<std::uintptr_t>::is_always_lock_free);
static_assert(std::atomic<std::uint32_t>::is_always_lock_free);

std::array<AudioThreadSlot, kAudioThreadSlotCount> gAudioThreadSlots {};
std::atomic<std::uint32_t> gAudioThreadSlotOverflowDepth {};

template <typename Thread>
std::uintptr_t threadToken(Thread thread) noexcept BWS_NONBLOCKING
{
    if constexpr (std::is_pointer_v<Thread>)
        return reinterpret_cast<std::uintptr_t>(thread);
    else
        return static_cast<std::uintptr_t>(thread);
}

std::uintptr_t currentThreadToken() noexcept BWS_NONBLOCKING
{
#if defined(_WIN32)
    std::uintptr_t token {};
    BWS_NONBLOCKING_UNSAFE(token = static_cast<std::uintptr_t>(::GetCurrentThreadId());)
    return token;
#else
    pthread_t thread {};
    // pthread_self is a bounded native thread-token query.
    BWS_NONBLOCKING_UNSAFE(thread = ::pthread_self();)
    return threadToken(thread);
#endif
}

} // namespace

AudioThreadScope::AudioThreadScope() noexcept BWS_NONBLOCKING
{
    const auto token = currentThreadToken();
    const auto start = token % kAudioThreadSlotCount;
    for (std::size_t offset = 0; offset < kAudioThreadSlotCount; ++offset)
    {
        const auto index = (start + offset) % kAudioThreadSlotCount;
        auto& slot = gAudioThreadSlots[index];
        auto expected = std::uintptr_t {};
        if (slot.token.load(std::memory_order_acquire) == token ||
            slot.token.compare_exchange_strong(expected, token, std::memory_order_acq_rel, std::memory_order_acquire))
        {
            slot.depth.fetch_add(1, std::memory_order_acq_rel);
            slotIndex_ = static_cast<std::uint32_t>(index);
            return;
        }
    }

    // Capacity exhaustion is exceptionally unlikely. Fail conservatively:
    // while an untracked RT scope exists, every thread is treated as RT.
    gAudioThreadSlotOverflowDepth.fetch_add(1, std::memory_order_acq_rel);
    overflowed_ = true;
}

AudioThreadScope::~AudioThreadScope() noexcept BWS_NONBLOCKING
{
    if (overflowed_)
    {
        gAudioThreadSlotOverflowDepth.fetch_sub(1, std::memory_order_acq_rel);
        return;
    }

    if (slotIndex_ == kUntrackedSlot)
        return;

    auto& slot = gAudioThreadSlots[slotIndex_];
    if (slot.depth.fetch_sub(1, std::memory_order_acq_rel) == 1)
        slot.token.store(0, std::memory_order_release);
}

bool AudioThreadScope::isAudioThread() noexcept BWS_NONBLOCKING
{
    if (gAudioThreadSlotOverflowDepth.load(std::memory_order_acquire) > 0)
        return true;

    const auto token = currentThreadToken();
    for (const auto& slot : gAudioThreadSlots)
    {
        if (slot.token.load(std::memory_order_acquire) == token)
            return slot.depth.load(std::memory_order_acquire) > 0;
    }
    return false;
}

} // namespace bws::rt
