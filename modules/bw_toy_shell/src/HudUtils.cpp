// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_toy_shell/HudUtils.h"

#include <algorithm>
#include <cmath>

namespace bws::toy_shell
{

juce::String fmtTime4(double seconds)
{
    const int s = static_cast<int>(std::max(0.0, std::floor(seconds)));
    return juce::String(s).paddedLeft('0', 4);
}

void paintSolvedOverlay(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour wash, juce::Colour text,
                        float fade01, juce::StringRef textToShow)
{
    const float fade = std::clamp(fade01, 0.0f, 1.0f);
    g.setColour(wash.withAlpha(fade * 0.45f));
    g.fillRect(bounds);
    g.setColour(text.withAlpha(fade));
    g.setFont(juce::FontOptions(32.0f).withStyle("Bold"));
    g.drawText(textToShow, bounds, juce::Justification::centred, false);
}

} // namespace bws::toy_shell
