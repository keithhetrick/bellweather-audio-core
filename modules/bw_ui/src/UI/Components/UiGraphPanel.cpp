// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/Components/UiGraphPanel.h"
#include "bw_ui/adapters/UiLayoutMetrics.h"
#include "bw_ui/generated/BwTokens.h"

namespace bws::ui
{

UiGraphPanel::UiGraphPanel(const UiThemeResolved& themeIn, bool denseIn)
    : theme(themeIn)
    , kernelTheme(adapters::makeKernelThemeSnapshot(themeIn))
    , titleLabel(UiTypographyRole::SectionLabel, themeIn)
    , dense(denseIn)
{
    setInterceptsMouseClicks(false, false);
    titleLabel.setKernelTheme(kernelTheme);
    titleLabel.setTextRole(kernel::TextRole::SectionLabel);
    titleLabel.setUseSecondaryColour(true);
    addAndMakeVisible(titleLabel);
}

void UiGraphPanel::setTheme(const UiThemeResolved& newTheme)
{
    theme = newTheme;
    kernelTheme = adapters::makeKernelThemeSnapshot(newTheme);
    titleLabel.setTheme(newTheme);
    titleLabel.setKernelTheme(kernelTheme);
    repaint();
}

void UiGraphPanel::setScale(float newScale)
{
    scale = newScale;
    titleLabel.setScale(newScale);
    resized();
}

void UiGraphPanel::setTitle(const juce::String& newTitle)
{
    title = newTitle;
    titleLabel.setText(title, juce::dontSendNotification);
    titleLabel.setVisible(title.isNotEmpty());
    resized();
}

void UiGraphPanel::setDense(bool shouldBeDense)
{
    dense = shouldBeDense;
    resized();
}

int UiGraphPanel::padding() const
{
    return adapters::panelPadding(kernelTheme, dense, scale);
}

int UiGraphPanel::titleGap() const
{
    return adapters::panelTitleGap(kernelTheme, scale);
}

float UiGraphPanel::cornerRadius() const
{
    return theme.surfaces.radiusMd * scale;
}

juce::Rectangle<int> UiGraphPanel::getContentBounds() const noexcept
{
    return contentArea;
}

void UiGraphPanel::updateLayout()
{
    auto bounds = getLocalBounds();
    bounds.reduce(padding(), padding());

    if (title.isNotEmpty())
    {
        const int titleH = adapters::sectionTextHeight(kernelTheme, kernel::TextRole::SectionLabel, scale);
        auto titleArea = bounds.removeFromTop(titleH);
        titleLabel.setBounds(titleArea);
        bounds.removeFromTop(titleGap());
    }
    else
    {
        titleLabel.setBounds(juce::Rectangle<int>());
    }

    contentArea = bounds;
}

void UiGraphPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.setColour(theme.colors.bg1);
    g.fillRoundedRectangle(bounds, cornerRadius());

    g.setColour(theme.colors.outline0.withAlpha(bws::tokens::shared::opacity::graph_panel::OUTLINE));
    g.drawRoundedRectangle(bounds, cornerRadius(), theme.surfaces.strokeThin);

    // Divider between title and content (if title present)
    if (title.isNotEmpty())
    {
        const int pad = padding();
        const int gap = titleGap();
        const int titleH = adapters::sectionTextHeight(kernelTheme, kernel::TextRole::SectionLabel, scale);
        const float y = (float)(pad + titleH + gap * 0.5f);
        g.setColour(theme.colors.outline0.withAlpha(bws::tokens::shared::opacity::graph_panel::DIVIDER));
        g.drawHorizontalLine((int)y, bounds.getX() + pad, bounds.getRight() - pad);
    }
}

void UiGraphPanel::resized()
{
    updateLayout();
}

} // namespace bws::ui
