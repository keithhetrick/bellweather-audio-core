// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

/// @file
/// @brief Debug-only thread-affinity assertions (e.g. not-on-audio-thread).

#include <cassert>
#include "bw_rt/AudioThreadScope.h"

namespace bws::rt
{

inline void assertNotAudioThread()
{
    // Only active in debug builds; avoids RT hazards on filesystem walks.
    assert(!AudioThreadScope::isAudioThread() && "Not safe to run on audio thread");
}

} // namespace bws::rt
