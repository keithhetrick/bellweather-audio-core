// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/Components/UiHelpPopover.h"
#include "bw_ui/generated/BwTokens.h"

namespace bws::ui
{

UiHelpPopover::UiHelpPopover(const UiThemeResolved& themeIn)
    : theme(themeIn)
    , kernelTheme(adapters::makeKernelThemeSnapshot(themeIn))
{
    setInterceptsMouseClicks(false, false);
    setRepaintsOnMouseActivity(false);
}

void UiHelpPopover::setText(const juce::String& newText)
{
    if (text == newText)
        return;
    text = newText;
    repaint();
}

void UiHelpPopover::setDense(bool shouldBeDense)
{
    if (dense == shouldBeDense)
        return;
    dense = shouldBeDense;
    repaint();
}

void UiHelpPopover::setTheme(const UiThemeResolved& newTheme)
{
    theme = newTheme;
    kernelTheme = adapters::makeKernelThemeSnapshot(newTheme);
    repaint();
}

void UiHelpPopover::setScale(float newScale)
{
    if (juce::approximatelyEqual(scale, newScale))
        return;
    scale = newScale;
    repaint();
}

int UiHelpPopover::padding() const
{
    return (int)std::round((dense ? kernelTheme.spacing.sm : kernelTheme.spacing.md) * scale);
}

float UiHelpPopover::radius() const
{
    return theme.surfaces.radiusSm * scale;
}

juce::Font UiHelpPopover::font() const
{
    return adapters::makeFont(kernelTheme, kernel::TextRole::ControlText, scale);
}

juce::Rectangle<int> UiHelpPopover::preferredSize(int maxWidth) const
{
    auto f = font();
    const int pad = padding();
    const int lineHeight = (int)std::ceil(f.getHeight());
    const int maxTextWidth = juce::jmax(1, maxWidth - pad * 2);
    juce::GlyphArrangement ga;
    ga.addJustifiedText(f, text, 0.0f, 0.0f, (float)maxTextWidth, juce::Justification::topLeft, true);
    auto bounds = ga.getBoundingBox(0, ga.getNumGlyphs(), true);
    const int textW = (int)std::ceil(bounds.getWidth());
    const int textH = (int)std::ceil(bounds.getHeight());
    const int width = juce::jmax(textW, maxTextWidth);
    return juce::Rectangle<int>(0, 0, width + pad * 2,
                                textH + pad * 2 + lineHeight * (text.containsChar('\n') ? 0 : 1));
}

void UiHelpPopover::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    const int pad = padding();
    bounds = bounds.reduced((float)pad);

    g.setColour(theme.colors.bg1);
    g.fillRoundedRectangle(bounds, radius());

    g.setColour(theme.colors.outline0.withAlpha(bws::tokens::shared::opacity::outline::STANDARD));
    g.drawRoundedRectangle(bounds, radius(), theme.surfaces.strokeThin);

    g.setFont(font());
    g.setColour(theme.colors.text0);
    g.drawFittedText(adapters::applyTextCasing(kernelTheme, kernel::TextRole::ControlText, text), bounds.toNearestInt(),
                     juce::Justification::topLeft, 4);
}

} // namespace bws::ui
