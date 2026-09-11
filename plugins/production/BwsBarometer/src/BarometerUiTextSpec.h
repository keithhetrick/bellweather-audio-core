// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <juce_core/juce_core.h>

namespace bws::barometer::ui_text
{

inline juce::String tooltipForKey(const juce::String& key)
{
    if (key == "soloL")
        return "SOLO L - audition the left channel. Enabling it turns Solo R off. Stereo only.";
    if (key == "swapLR")
        return "SWAP L/R - exchange the stereo channels. Independent of Solo. Stereo only.";
    if (key == "soloR")
        return "SOLO R - audition the right channel. Enabling it turns Solo L off. Stereo only.";
    if (key == "measurementReset")
        return "RESET - restart Integrated, LRA, Max-M, Max-S, and held true peak.";
    return {};
}

} // namespace bws::barometer::ui_text
