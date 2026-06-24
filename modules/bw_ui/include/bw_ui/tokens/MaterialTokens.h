// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <juce_graphics/juce_graphics.h>
#include "ShadowTokens.h"

namespace bws::ui::tokens
{

/**
 * Material finish types for surface rendering.
 */
enum class MaterialFinish
{
    Matte,   // No shine (Sage theme)
    Glossy,  // High shine (Vivid theme)
    Brushed, // Directional grain (Ember theme)
    Worn     // Weathered/oxidized (Apocalypse Barbie, Patina)
};

/**
 * Texture overlay types for surface detail.
 */
enum class TextureType
{
    None,       // No texture
    FineGrain,  // Subtle noise (all themes)
    MetalGrain, // Brushed metal lines (Ember, Patina)
    Scratched,  // Wear marks (Apocalypse Barbie)
    Weathered   // Oxidation/patina (Patina)
};

/**
 * Complete material definition for UI surfaces.
 */
struct Material
{
    juce::Colour fill;
    juce::Colour border;
    float borderWidth = 0.0f;
    float cornerRadius = 0.0f;
    Shadow shadow = {};
    float opacity = 1.0f;

    // Rendering properties
    MaterialFinish finish = MaterialFinish::Matte;
    TextureType texture = TextureType::None;
    float textureStrength = 0.0f; // 0.0-1.0
    float specularity = 0.0f;     // 0.0-1.0 (shine intensity)
    float bloomStrength = 0.0f;   // 0.0-1.0 (glow intensity)

    Material() = default;
};

} // namespace bws::ui::tokens
