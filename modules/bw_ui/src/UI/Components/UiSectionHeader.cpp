// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/Components/UiSectionHeader.h"
#include "bw_ui/adapters/UiLayoutMetrics.h"

namespace bws::ui
{

UiSectionHeader::UiSectionHeader(const UiThemeResolved& themeIn, juce::String titleText,
                                 std::optional<juce::String> subtitleText, bool denseIn)
    : theme(themeIn)
    , kernelTheme(adapters::makeKernelThemeSnapshot(themeIn))
    , titleLabel(UiTypographyRole::Title, themeIn)
    , subtitleLabel(UiTypographyRole::ControlText, themeIn)
    , title(std::move(titleText))
    , subtitle(std::move(subtitleText))
    , dense(denseIn)
{
    titleLabel.setKernelTheme(kernelTheme);
    titleLabel.setTextRole(kernel::TextRole::Title);
    subtitleLabel.setKernelTheme(kernelTheme);
    subtitleLabel.setTextRole(kernel::TextRole::ControlText);
    subtitleLabel.setUseSecondaryColour(true);
    titleLabel.setText(title, juce::dontSendNotification);
    if (subtitle)
    {
        subtitleLabel.setText(*subtitle, juce::dontSendNotification);
        addAndMakeVisible(subtitleLabel);
    }
    addAndMakeVisible(titleLabel);
}

void UiSectionHeader::setTheme(const UiThemeResolved& newTheme)
{
    theme = newTheme;
    kernelTheme = adapters::makeKernelThemeSnapshot(newTheme);
    titleLabel.setTheme(newTheme);
    titleLabel.setKernelTheme(kernelTheme);
    subtitleLabel.setTheme(newTheme);
    subtitleLabel.setKernelTheme(kernelTheme);
    resized();
}

void UiSectionHeader::setScale(float newScale)
{
    scale = newScale;
    titleLabel.setScale(newScale);
    subtitleLabel.setScale(newScale);
    resized();
}

void UiSectionHeader::setTitle(juce::String newTitle)
{
    title = std::move(newTitle);
    titleLabel.setText(title, juce::dontSendNotification);
    resized();
}

void UiSectionHeader::setSubtitle(std::optional<juce::String> newSubtitle)
{
    subtitle = std::move(newSubtitle);
    if (subtitle && subtitle->isNotEmpty())
    {
        subtitleLabel.setText(*subtitle, juce::dontSendNotification);
        if (!subtitleLabel.isVisible())
            addAndMakeVisible(subtitleLabel);
    }
    else
    {
        subtitleLabel.setVisible(false);
    }
    resized();
}

void UiSectionHeader::setDense(bool shouldBeDense)
{
    dense = shouldBeDense;
    resized();
}

int UiSectionHeader::verticalPadding() const
{
    return adapters::sectionVerticalPadding(kernelTheme, dense, scale);
}

int UiSectionHeader::horizontalPadding() const
{
    return adapters::sectionHorizontalPadding(kernelTheme, scale);
}

int UiSectionHeader::gap() const
{
    return adapters::sectionGap(kernelTheme, scale);
}

int UiSectionHeader::preferredHeight(bool denseMode, float scaleValue, const UiThemeResolved& themeIn, bool hasSubtitle)
{
    const auto kernelTheme = adapters::makeKernelThemeSnapshot(themeIn);
    return adapters::sectionPreferredHeight(kernelTheme, denseMode, scaleValue, hasSubtitle);
}

int UiSectionHeader::preferredHeight() const
{
    return preferredHeight(dense, scale, theme, subtitle.has_value());
}

void UiSectionHeader::resized()
{
    auto bounds = getLocalBounds();
    bounds.reduce(horizontalPadding(), verticalPadding());

    const int titleH = adapters::sectionTextHeight(kernelTheme, kernel::TextRole::Title, scale);
    const int subtitleH = adapters::sectionTextHeight(kernelTheme, kernel::TextRole::ControlText, scale);

    const int gapPx = (subtitle && subtitle->isNotEmpty()) ? gap() : 0;

    auto area = bounds;
    titleLabel.setBounds(area.removeFromTop(titleH));

    if (subtitle && subtitle->isNotEmpty())
    {
        const int gapSize = juce::jlimit(0, area.getHeight(), gapPx);
        area.removeFromTop(gapSize);
        subtitleLabel.setBounds(area.removeFromTop(subtitleH));
        subtitleLabel.setVisible(true);
    }
    else
    {
        subtitleLabel.setVisible(false);
    }
}

} // namespace bws::ui
