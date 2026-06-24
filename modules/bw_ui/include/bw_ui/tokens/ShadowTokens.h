// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cmath>
#include <juce_graphics/juce_graphics.h>
#include "bw_ui/generated/BwTokens.h"

namespace bws::ui::tokens
{

/**
 * Shadow definition for elevation and depth effects.
 */
struct Shadow
{
    int offsetX = 0;
    int offsetY = 0;
    float blur = 0.0f;
    juce::Colour color;

    Shadow() = default;
    Shadow(int x, int y, float b, juce::Colour c)
        : offsetX(x)
        , offsetY(y)
        , blur(b)
        , color(c)
    {}

    bool operator==(const Shadow& other) const
    {
        return offsetX == other.offsetX && offsetY == other.offsetY && std::abs(blur - other.blur) < 0.0001f &&
               color == other.color;
    }

    bool operator!=(const Shadow& other) const { return !(*this == other); }
};

// Construct a Shadow from a generated rendering shadow token, wrapping the ARGB
// uint32_t into a juce::Colour at the edge. The elevation/inner/glow/coloredGlow
// value bank lives in bws::tokens::rendering::shadows::* (generated from
// bds/tokens/products/rendering.tokens.json).
inline Shadow shadowFromToken(const ::bws::tokens::rendering::shadows::ShadowToken& t)
{
    return Shadow {t.offsetX, t.offsetY, t.blur, juce::Colour(t.color)};
}

} // namespace bws::ui::tokens
