// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

/// @file
/// @brief RAII marker identifying the current scope as the audio thread.

#include <atomic>
#include <cstdint>
#include "bw_rt/RtSafety.h"

namespace bws::rt
{

class AudioThreadScope
{
public:
    AudioThreadScope() noexcept BWS_NONBLOCKING;
    ~AudioThreadScope() noexcept BWS_NONBLOCKING;

    AudioThreadScope(const AudioThreadScope&) = delete;
    AudioThreadScope& operator=(const AudioThreadScope&) = delete;

    static bool isAudioThread() noexcept BWS_NONBLOCKING;

private:
    static constexpr std::uint32_t kUntrackedSlot = UINT32_MAX;

    std::uint32_t slotIndex_ {kUntrackedSlot};
    bool overflowed_ {};
};

} // namespace bws::rt
