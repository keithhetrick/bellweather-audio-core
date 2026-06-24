// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bw_state_types/SystemInfo.h>

namespace bws::adapters
{

// Build a bw::SystemInfo snapshot from the current process environment.
//
// Implementation lives in bw_juce_adapters/src/CaptureSystemInfo.cpp and uses
// juce::SystemStats / juce::File to read the values. JUCE is confined to that
// single .cpp; no JUCE class symbol is exposed across the public surface.
//
// Call once at composition-root time and inject the result into consumers.
// The factory shape keeps the public adapter surface small.
bw::SystemInfo captureSystemInfo();

} // namespace bws::adapters
