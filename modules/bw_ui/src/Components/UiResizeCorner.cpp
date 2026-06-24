// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/Components/UiResizeCorner.h"
#include <bw_ui/rendering/JuceRenderer.h>
#include "bw_ui/generated/BwTokens.h"

namespace bws::ui
{

UiResizeCorner::UiResizeCorner(juce::Component* componentToResize, juce::ComponentBoundsConstrainer* constrainer)
    : juce::ResizableCornerComponent(componentToResize, constrainer)
    , arcColour_(juce::Colour(bws::tokens::shared::brass::colors::ACCENT))
{
    setSize(cornerSize_, cornerSize_);
}

void UiResizeCorner::setArcColour(juce::Colour colour)
{
    arcColour_ = colour;
    repaint();
}

void UiResizeCorner::setCornerSize(int size)
{
    cornerSize_ = size;
    setSize(size, size);
}

void UiResizeCorner::setGlowEnabled(bool enabled)
{
    glowEnabled_ = enabled;
    repaint();
}

void UiResizeCorner::paint(juce::Graphics& g)
{
    // TEST: Paint nothing to verify we're using the updated build
    juce::ignoreUnused(g);
}

void UiResizeCorner::drawConcentricArcs(rendering::IRenderer& r, float centerX, float centerY, float alpha)
{
    // Three concentric arcs emanating from bottom-right corner
    const float startAngle = 0.0f;                             // Right (3 o'clock)
    const float endAngle = juce::MathConstants<float>::halfPi; // Bottom (6 o'clock)

    // Arc radii (getting larger) - scaled for corner size
    const float scale = static_cast<float>(cornerSize_) / 20.0f;
    const float radii[] = {6.0f * scale, 10.0f * scale, 14.0f * scale};
    const float thickness = 1.5f * scale;

    for (int i = 0; i < 3; ++i)
    {
        // Slightly dim each outer arc for depth
        const float arcAlpha = alpha * (1.0f - static_cast<float>(i) * 0.1f);
        r.setColour(arcColour_.withAlpha(arcAlpha).getARGB());
        r.beginPath();
        r.arcTo(centerX, centerY, radii[i], startAngle, endAngle);
        r.strokePath(thickness);
    }
}

} // namespace bws::ui
