// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <juce_graphics/juce_graphics.h>
#include "bw_ui/tokens/ShadowTokens.h"

namespace bws::ui::rendering
{

/**
 * Renders shadows and glow effects for UI elements.
 * Supports drop shadows, inner shadows, and colored glows.
 */
class ShadowRenderer
{
public:
    ShadowRenderer() = default;

    /**
     * Render drop shadow (elevation effect).
     * Should be called BEFORE rendering the main shape.
     */
    static void renderDropShadow(juce::Graphics& g, const juce::Rectangle<float>& bounds, const tokens::Shadow& shadow,
                                 float cornerRadius = 0.0f);

    /**
     * Render inner shadow (recessed effect).
     * Should be called AFTER rendering the main shape.
     */
    static void renderInnerShadow(juce::Graphics& g, const juce::Rectangle<float>& bounds, const tokens::Shadow& shadow,
                                  float cornerRadius = 0.0f);

    /**
     * Render glow effect (colored halo).
     * Should be called BEFORE rendering the main shape.
     */
    static void renderGlow(juce::Graphics& g, const juce::Rectangle<float>& bounds, const tokens::Shadow& glow,
                           float cornerRadius = 0.0f);

private:
    // Helper: create shadow path
    static juce::Path createShadowPath(const juce::Rectangle<float>& bounds, float cornerRadius);
};

} // namespace bws::ui::rendering
