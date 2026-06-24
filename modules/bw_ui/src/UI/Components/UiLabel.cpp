// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/Components/UiLabel.h"

namespace bws::ui
{

UiLabel::UiLabel(UiTypographyRole roleIn, const UiThemeResolved& themeIn)
    : role(roleIn)
    , theme(themeIn)
{}

void UiLabel::setText(const juce::String& newText, juce::NotificationType)
{
    text = newText;
    repaint();
}

void UiLabel::setRole(UiTypographyRole newRole)
{
    role = newRole;
    repaint();
}

void UiLabel::setTheme(const UiThemeResolved& newTheme)
{
    theme = newTheme;
    repaint();
}

void UiLabel::setScale(float newScale)
{
    scale = newScale;
    repaint();
}

void UiLabel::setKernelTheme(const kernel::ThemeSnapshot& newTheme)
{
    kernelTheme = newTheme;
    repaint();
}

void UiLabel::setTextRole(kernel::TextRole newRole)
{
    kernelRole = newRole;
    repaint();
}

void UiLabel::setUseSecondaryColour(bool shouldUseSecondary)
{
    useSecondary = shouldUseSecondary;
    repaint();
}

void UiLabel::paint(juce::Graphics& g)
{
    const auto resolvedKernelTheme = kernelTheme.value_or(adapters::makeKernelThemeSnapshot(theme));
    const auto resolvedKernelRole = kernelRole.value_or(adapters::toKernelTextRole(role));
    auto font = adapters::makeFont(resolvedKernelTheme, resolvedKernelRole, scale);
    auto displayText = adapters::applyTextCasing(resolvedKernelTheme, resolvedKernelRole, text);

    g.setFont(font);
    g.setColour(useSecondary ? theme.colors.text1 : theme.colors.text0);
    g.drawFittedText(displayText, getLocalBounds(), juce::Justification::centred, 1);
}

} // namespace bws::ui
