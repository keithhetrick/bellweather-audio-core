// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/rendering/MaterialRenderer.h"

namespace bws::ui::rendering
{

MaterialRenderer::MaterialRenderer() = default;

void MaterialRenderer::render(juce::Graphics& g, const juce::Rectangle<float>& bounds, const tokens::Material& material)
{
    if (material.cornerRadius > 0.0f)
    {
        renderRounded(g, bounds, material);
    }
    else
    {
        // 1. Drop shadow / glow
        if (material.bloomStrength > 0.0f)
        {
            tokens::Shadow glowShadow = material.shadow;
            glowShadow.color = glowShadow.color.withAlpha(material.bloomStrength);
            ShadowRenderer::renderGlow(g, bounds, glowShadow, material.cornerRadius);
        }
        else
        {
            ShadowRenderer::renderDropShadow(g, bounds, material.shadow, material.cornerRadius);
        }

        // 2. Fill
        renderFill(g, bounds, material);

        // 3. Texture overlay
        if (material.texture != tokens::TextureType::None)
        {
            textureRenderer.renderTexture(g, bounds, material.texture, material.textureStrength, material.cornerRadius);
        }

        // 4. Border
        if (material.borderWidth > 0.0f)
        {
            renderBorder(g, bounds, material);
        }
    }
}

void MaterialRenderer::renderRounded(juce::Graphics& g, const juce::Rectangle<float>& bounds,
                                     const tokens::Material& material)
{
    // Same as render() but uses rounded rectangle path

    // 1. Shadow/glow
    if (material.bloomStrength > 0.0f)
    {
        tokens::Shadow glowShadow = material.shadow;
        glowShadow.color = glowShadow.color.withAlpha(material.bloomStrength);
        ShadowRenderer::renderGlow(g, bounds, glowShadow, material.cornerRadius);
    }
    else
    {
        ShadowRenderer::renderDropShadow(g, bounds, material.shadow, material.cornerRadius);
    }

    // 2. Fill
    g.setColour(material.fill.withAlpha(material.opacity));
    g.fillRoundedRectangle(bounds, material.cornerRadius);

    // 3. Texture
    if (material.texture != tokens::TextureType::None)
    {
        textureRenderer.renderTexture(g, bounds, material.texture, material.textureStrength, material.cornerRadius);
    }

    // 4. Border
    if (material.borderWidth > 0.0f)
    {
        g.setColour(material.border.withAlpha(material.opacity));
        g.drawRoundedRectangle(bounds, material.cornerRadius, material.borderWidth);
    }
}

void MaterialRenderer::renderEllipse(juce::Graphics& g, const juce::Rectangle<float>& bounds,
                                     const tokens::Material& material)
{
    // 1. Shadow/glow
    if (material.bloomStrength > 0.0f)
    {
        tokens::Shadow glowShadow = material.shadow;
        glowShadow.color = glowShadow.color.withAlpha(material.bloomStrength);
        ShadowRenderer::renderGlow(g, bounds, glowShadow, 0.0f);
    }
    else
    {
        ShadowRenderer::renderDropShadow(g, bounds, material.shadow, 0.0f);
    }

    // 2. Fill
    g.setColour(material.fill.withAlpha(material.opacity));
    g.fillEllipse(bounds);

    // 3. Texture (clipped to ellipse)
    if (material.texture != tokens::TextureType::None)
    {
        juce::Graphics::ScopedSaveState saveState(g);
        juce::Path clipPath;
        clipPath.addEllipse(bounds);
        g.reduceClipRegion(clipPath);

        textureRenderer.renderTexture(g, bounds, material.texture, material.textureStrength, 0.0f);
    }

    // 4. Border
    if (material.borderWidth > 0.0f)
    {
        g.setColour(material.border.withAlpha(material.opacity));
        g.drawEllipse(bounds.reduced(material.borderWidth * 0.5f), material.borderWidth);
    }
}

void MaterialRenderer::renderFill(juce::Graphics& g, const juce::Rectangle<float>& bounds,
                                  const tokens::Material& material)
{
    g.setColour(material.fill.withAlpha(material.opacity));
    g.fillRect(bounds);
}

void MaterialRenderer::renderBorder(juce::Graphics& g, const juce::Rectangle<float>& bounds,
                                    const tokens::Material& material)
{
    g.setColour(material.border.withAlpha(material.opacity));
    g.drawRect(bounds, material.borderWidth);
}

void MaterialRenderer::clearCaches()
{
    textureRenderer.clearCache();
}

} // namespace bws::ui::rendering
