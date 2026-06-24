// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <juce_graphics/juce_graphics.h>
#include "bw_ui/tokens/MaterialTokens.h"

namespace bws::ui::rendering
{

/**
 * Renders texture overlays (grain, scratches, weathering).
 * Uses cached noise patterns for performance.
 */
class TextureRenderer
{
public:
    TextureRenderer();

    /**
     * Render texture overlay on a surface.
     */
    void renderTexture(juce::Graphics& g, const juce::Rectangle<float>& bounds, tokens::TextureType type,
                       float strength, float cornerRadius = 0.0f);

    /**
     * Clear cached textures (call when memory is constrained).
     */
    void clearCache();

private:
    // Cached noise images
    juce::Image fineGrainCache;
    juce::Image metalGrainCache;
    juce::Image scratchedCache;
    juce::Image weatheredCache;

    // Generate noise patterns
    juce::Image generateFineGrain(int width, int height);
    juce::Image generateMetalGrain(int width, int height);
    juce::Image generateScratches(int width, int height);
    juce::Image generateWeathering(int width, int height);

    // Get or create cached texture
    juce::Image& getCachedTexture(tokens::TextureType type, int width, int height);
};

} // namespace bws::ui::rendering
