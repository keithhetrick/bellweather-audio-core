// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/Components/UiPanel.h"
#include "bw_ui/generated/BwTokens.h"

namespace bws::ui
{

UiPanel::UiPanel(const UiThemeResolved& themeIn)
    : theme(themeIn)
{
    setOpaque(true);
    setInterceptsMouseClicks(false, false);
}

void UiPanel::setTheme(const UiThemeResolved& newTheme)
{
    theme = newTheme;
    repaint();
}

void UiPanel::setScale(float newScale)
{
    scale = newScale;
    repaint();
}

void UiPanel::setFramed(bool shouldFrame)
{
    framed = shouldFrame;
    repaint();
}

void UiPanel::setTreatment(Treatment newTreatment)
{
    treatment = newTreatment;
    repaint();
}

float UiPanel::framePadding() const
{
    return framed ? theme.spacing.sm * scale : 0.0f;
}

void UiPanel::paintMatte(juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    auto lighter = theme.colors.bg1.brighter(0.06f);
    juce::ColourGradient grad(lighter, bounds.getX(), bounds.getY(), theme.colors.bg1, bounds.getX(),
                              bounds.getBottom(), false);
    namespace op = bws::tokens::shared::opacity::ui_panel;
    grad.addColour(0.55f, theme.colors.bg2.withAlpha(op::MATTE_MID));
    g.setGradientFill(grad);
    g.fillRoundedRectangle(bounds, theme.surfaces.radiusLg);

    g.setColour(theme.colors.outline0.withAlpha(op::MATTE_OUTLINE));
    g.drawRoundedRectangle(bounds, theme.surfaces.radiusLg, theme.surfaces.strokeThin);
}

void UiPanel::paintBrushed(juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    paintMatte(g, bounds);
    const float stripeSpacing = juce::jmax(4.0f, 10.0f * scale);
    namespace op = bws::tokens::shared::opacity::ui_panel;
    g.setColour(theme.colors.text1.withAlpha(op::STRIPE_VERTICAL));
    for (float x = bounds.getX(); x < bounds.getRight(); x += stripeSpacing)
        g.drawVerticalLine((int)x, bounds.getY(), bounds.getBottom());

    g.setColour(theme.colors.text1.withAlpha(op::STRIPE_HORIZONTAL));
    for (float y = bounds.getY(); y < bounds.getBottom(); y += stripeSpacing * 0.6f)
        g.drawHorizontalLine((int)y, bounds.getX(), bounds.getRight());
}

void UiPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.fillAll(theme.colors.bg0);

    const float pad = framePadding();
    if (pad > 0.0f)
        bounds = bounds.reduced(pad);

    if (treatment == Treatment::EmbeddedBed)
    {
        auto bed = bounds;
        namespace op = bws::tokens::shared::opacity::ui_panel;
        juce::ColourGradient grad(theme.colors.bg0.withAlpha(op::BED_GRADIENT_START), bed.getX(), bed.getY(),
                                  theme.colors.bg0.withAlpha(op::BED_GRADIENT_END), bed.getX(), bed.getBottom(), false);
        grad.addColour(0.5f, theme.colors.bg1.withAlpha(op::BED_GRADIENT_MID));
        g.setGradientFill(grad);
        g.fillRoundedRectangle(bed, theme.surfaces.radiusLg);

        const auto topEdge = theme.colors.text1.withAlpha(op::BED_TOP_EDGE);
        const auto bottomShade = theme.colors.bg0.withAlpha(op::BED_BOTTOM_SHADE);
        g.setColour(topEdge);
        g.drawHorizontalLine((int)std::round(bed.getY()), bed.getX() + theme.surfaces.radiusLg * 0.7f,
                             bed.getRight() - theme.surfaces.radiusLg * 0.7f);
        g.setColour(bottomShade);
        g.drawHorizontalLine((int)std::round(bed.getBottom() - 1.0f), bed.getX() + theme.surfaces.radiusLg * 0.8f,
                             bed.getRight() - theme.surfaces.radiusLg * 0.8f);

        return;
    }

    switch (theme.surfaces.finish)
    {
    case UiPanelFinish::BrushedMetal:
        paintBrushed(g, bounds);
        break;
    case UiPanelFinish::Matte:
    default:
        paintMatte(g, bounds);
        break;
    }
}

// Factory methods
std::unique_ptr<UiPanel> UiPanel::xs(const UiThemeResolved& theme)
{
    auto panel = std::make_unique<UiPanel>(theme);
    panel->setSizeVariant(ComponentSize::XS);
    return panel;
}

std::unique_ptr<UiPanel> UiPanel::sm(const UiThemeResolved& theme)
{
    auto panel = std::make_unique<UiPanel>(theme);
    panel->setSizeVariant(ComponentSize::SM);
    return panel;
}

std::unique_ptr<UiPanel> UiPanel::md(const UiThemeResolved& theme)
{
    auto panel = std::make_unique<UiPanel>(theme);
    panel->setSizeVariant(ComponentSize::MD);
    return panel;
}

std::unique_ptr<UiPanel> UiPanel::lg(const UiThemeResolved& theme)
{
    auto panel = std::make_unique<UiPanel>(theme);
    panel->setSizeVariant(ComponentSize::LG);
    return panel;
}

std::unique_ptr<UiPanel> UiPanel::xl(const UiThemeResolved& theme)
{
    auto panel = std::make_unique<UiPanel>(theme);
    panel->setSizeVariant(ComponentSize::XL);
    return panel;
}

void UiPanel::setSizeVariant(ComponentSize size)
{
    currentSizeVariant = size;
    applySizeVariant(size);
}

void UiPanel::applySizeVariant(ComponentSize size)
{
    // Map size enum to actual minimum dimensions from theme
    int minWidth = 400;
    int minHeight = 300;

    switch (size)
    {
    case ComponentSize::XS:
        minWidth = 200;
        minHeight = 150;
        break;
    case ComponentSize::SM:
        minWidth = 300;
        minHeight = 200;
        break;
    case ComponentSize::MD:
        minWidth = 400;
        minHeight = 300;
        break;
    case ComponentSize::LG:
        minWidth = 600;
        minHeight = 450;
        break;
    case ComponentSize::XL:
        minWidth = 800;
        minHeight = 600;
        break;
    }

    // Set minimum size for panels (they can grow beyond this)
    Component::setSize(minWidth, minHeight);
}

} // namespace bws::ui
