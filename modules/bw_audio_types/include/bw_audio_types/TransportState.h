// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstdint>

namespace bws::domain
{

/**
 * Host transport information in a framework-neutral shape. Adapters fill this
 * structure from the active host transport source. The engine reads it.
 */
struct TransportState
{
    double sampleRate = 44100.0;
    int samplesPerBlock = 512;
    double bpm = 120.0;
    double ppqPosition = 0.0;
    bool isPlaying = false;
    int64_t samplePosition = 0;
};

} // namespace bws::domain
