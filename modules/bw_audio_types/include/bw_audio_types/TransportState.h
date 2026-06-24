// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstdint>

namespace bws::domain
{

/**
 * Host transport information. Neutral wrapper over JUCE AudioPlayHead PositionInfo.
 * The JUCE adapter fills this from AudioPlayHead. The engine reads it.
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
