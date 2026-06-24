// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/rendering/ShadowRenderer.h"

namespace bws::ui::rendering
{

void ShadowRenderer::renderDropShadow(juce::Graphics& g, const juce::Rectangle<float>& bounds,
                                      const tokens::Shadow& shadow, float cornerRadius)
{
    if (shadow.blur <= 0.0f && shadow.color.isTransparent())
    {
        return; // No shadow to render
    }

    // Offset shadow bounds
    auto shadowBounds = bounds.translated(static_cast<float>(shadow.offsetX), static_cast<float>(shadow.offsetY));

    // Create shadow path
    juce::Path shadowPath;
    if (cornerRadius > 0.0f)
    {
        shadowPath.addRoundedRectangle(shadowBounds, cornerRadius);
    }
    else
    {
        shadowPath.addRectangle(shadowBounds);
    }

    // Apply blur and render
    g.setColour(shadow.color);

    if (shadow.blur > 0.0f)
    {
        // Approximate gaussian blur with multiple passes
        int passes = static_cast<int>(shadow.blur / 2.0f) + 1;
        float alphaPerPass = shadow.color.getFloatAlpha() / static_cast<float>(passes);

        for (int i = 0; i < passes; ++i)
        {
            float spread = static_cast<float>(i) * 0.5f;
            auto blurredBounds = shadowBounds.expanded(spread);

            juce::Path blurPath;
            if (cornerRadius > 0.0f)
            {
                blurPath.addRoundedRectangle(blurredBounds, cornerRadius + spread);
            }
            else
            {
                blurPath.addRectangle(blurredBounds);
            }

            g.setColour(shadow.color.withAlpha(alphaPerPass));
            g.fillPath(blurPath);
        }
    }
    else
    {
        g.fillPath(shadowPath);
    }
}

void ShadowRenderer::renderInnerShadow(juce::Graphics& g, const juce::Rectangle<float>& bounds,
                                       const tokens::Shadow& shadow, float cornerRadius)
{
    if (shadow.blur <= 0.0f && shadow.color.isTransparent())
    {
        return;
    }

    // Create clipping path for element
    juce::Path clipPath;
    if (cornerRadius > 0.0f)
    {
        clipPath.addRoundedRectangle(bounds, cornerRadius);
    }
    else
    {
        clipPath.addRectangle(bounds);
    }

    // Save graphics state
    juce::Graphics::ScopedSaveState saveState(g);

    // Clip to element bounds
    g.reduceClipRegion(clipPath);

    // Render inverted shadow
    auto shadowBounds = bounds.translated(static_cast<float>(shadow.offsetX), static_cast<float>(shadow.offsetY));

    // Expand bounds to create inner effect
    auto expandedBounds = bounds.expanded(shadow.blur * 2.0f);

    // Create outer path (fill)
    juce::Path outerPath;
    outerPath.addRectangle(expandedBounds);

    // Create inner path (subtract)
    juce::Path innerPath;
    if (cornerRadius > 0.0f)
    {
        innerPath.addRoundedRectangle(shadowBounds, cornerRadius);
    }
    else
    {
        innerPath.addRectangle(shadowBounds);
    }

    // Subtract inner from outer
    outerPath = outerPath.createPathWithRoundedCorners(0.0f);

    // Render with gradient for blur approximation
    juce::ColourGradient gradient(shadow.color, bounds.getCentreX(), bounds.getCentreY(), shadow.color.withAlpha(0.0f),
                                  bounds.getCentreX() + shadow.blur, bounds.getCentreY() + shadow.blur, true);

    g.setGradientFill(gradient);
    g.fillPath(outerPath);
}

void ShadowRenderer::renderGlow(juce::Graphics& g, const juce::Rectangle<float>& bounds, const tokens::Shadow& glow,
                                float cornerRadius)
{
    // Glow is just a drop shadow with no offset
    tokens::Shadow glowShadow = glow;
    glowShadow.offsetX = 0;
    glowShadow.offsetY = 0;

    renderDropShadow(g, bounds, glowShadow, cornerRadius);
}

juce::Path ShadowRenderer::createShadowPath(const juce::Rectangle<float>& bounds, float cornerRadius)
{
    juce::Path path;
    if (cornerRadius > 0.0f)
    {
        path.addRoundedRectangle(bounds, cornerRadius);
    }
    else
    {
        path.addRectangle(bounds);
    }
    return path;
}

} // namespace bws::ui::rendering
