// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <juce_graphics/juce_graphics.h>
#include "bw_ui/tokens/MaterialTokens.h"
#include "ShadowRenderer.h"
#include "TextureRenderer.h"

namespace bws::ui::rendering
{

/**
 * Main renderer for Material tokens.
 * Combines fill, border, shadow, texture, and effects.
 */
class MaterialRenderer
{
public:
    MaterialRenderer();

    /**
     * Render complete material to Graphics context.
     * Handles all material properties in correct order:
     * 1. Drop shadow / glow
     * 2. Fill
     * 3. Texture overlay
     * 4. Border
     * 5. Inner shadow
     */
    void render(juce::Graphics& g, const juce::Rectangle<float>& bounds, const tokens::Material& material);

    /**
     * Render material for rounded rectangle.
     */
    void renderRounded(juce::Graphics& g, const juce::Rectangle<float>& bounds, const tokens::Material& material);

    /**
     * Render material for ellipse/circle.
     */
    void renderEllipse(juce::Graphics& g, const juce::Rectangle<float>& bounds, const tokens::Material& material);

    /**
     * Clear texture caches.
     */
    void clearCaches();

private:
    TextureRenderer textureRenderer;

    // Render individual material properties
    void renderFill(juce::Graphics& g, const juce::Rectangle<float>& bounds, const tokens::Material& material);

    void renderBorder(juce::Graphics& g, const juce::Rectangle<float>& bounds, const tokens::Material& material);
};

} // namespace bws::ui::rendering
