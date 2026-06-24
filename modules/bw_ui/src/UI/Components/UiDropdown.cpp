// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/Components/UiDropdown.h"
#include "bw_ui/generated/BwTokens.h"

#include <algorithm>

namespace bws::ui
{

UiDropdown::DropdownLookAndFeel::DropdownLookAndFeel(const UiThemeResolved& theme, UiDropdownRole role)
    : ThemedPopupMenuLookAndFeel(theme, role)
{}

void UiDropdown::DropdownLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                                                   int buttonX, int buttonY, int buttonW, int buttonH,
                                                   juce::ComboBox& box)
{
    juce::ignoreUnused(isButtonDown, buttonX, buttonY, buttonW, buttonH);

    const auto& theme = getTheme();
    const auto role = getRole();
    const auto scale = getScale();
    const auto metrics = UiDropdowns::metrics(role, scale, theme);
    const bool isOverlay = (role == UiDropdownRole::Overlay);
    auto bounds = juce::Rectangle<float>(0.0f, 0.0f, (float)width, (float)height)
                      .reduced(bws::tokens::shared::geometry::STROKE_FULL_PX);

    // Overlay uses lighter bg alpha (70%) so the selector doesn't compete
    // with chart content behind it; Standard/Compact keep 92% saturation.
    const float bgAlpha = isOverlay ? 0.70f : 0.92f;
    g.setColour(theme.colors.bg2.withAlpha(bgAlpha));
    g.fillRoundedRectangle(bounds, metrics.cornerRadius);

    g.setColour(theme.colors.outline0.withAlpha(bws::tokens::shared::opacity::outline::EMPHASIZED));
    g.drawRoundedRectangle(bounds, metrics.cornerRadius, metrics.outlineThickness);

    const float arrowW = juce::jmin(metrics.arrowBoxWidth, bounds.getWidth() * 0.4f);
    auto arrowArea = bounds.removeFromRight(arrowW).reduced(metrics.padding * 0.35f);

    g.setColour(theme.colors.accent0);
    if (isOverlay)
    {
        // Filled triangle for Overlay - smaller footprint, reads correctly at
        // micro sizes. Matches MissionControlLookAndFeel chrome.
        const float arrowSize = std::max(1.5f, arrowArea.getHeight() * 0.42f);
        const float cx = arrowArea.getCentreX();
        const float topY = arrowArea.getCentreY() - (arrowSize * 0.4f);
        const float botY = arrowArea.getCentreY() + (arrowSize * 0.5f);
        juce::Path arrow;
        arrow.addTriangle(cx - arrowSize, topY, cx + arrowSize, topY, cx, botY);
        g.fillPath(arrow);
    }
    else
    {
        // Standard/UtilityMenu/Compact: V-stroke - traditional dropdown chevron.
        juce::Path arrow;
        arrow.startNewSubPath(arrowArea.getX(), arrowArea.getY());
        arrow.lineTo(arrowArea.getCentreX(), arrowArea.getBottom());
        arrow.lineTo(arrowArea.getRight(), arrowArea.getY());
        g.strokePath(arrow, juce::PathStrokeType(metrics.outlineThickness, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
    }

    auto textArea =
        juce::Rectangle<float>(metrics.padding, 0.0f, (float)width - metrics.padding - arrowW, (float)height);

    auto textColour = box.findColour(juce::ComboBox::textColourId);
    auto font = getComboBoxFont(box);

    g.setColour(textColour);
    g.setFont(font);
    g.drawText(box.getText(), textArea.toNearestInt(), juce::Justification::centredLeft, true);
}

void UiDropdown::DropdownLookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label)
{
    // Hide the internal Label since we draw text directly in drawComboBox.
    // Prevents double-rendering which causes garbled text like "Oofff".
    label.setVisible(false);
    juce::ignoreUnused(box);
}

juce::Font UiDropdown::DropdownLookAndFeel::getComboBoxFont(juce::ComboBox& box)
{
    // Role-aware: Overlay -> Annotation (9pt micro-chrome); others -> ControlText.
    const auto role = getRole();
    const auto textRole =
        (role == UiDropdownRole::Overlay) ? kernel::TextRole::Annotation : kernel::TextRole::ControlText;
    juce::ignoreUnused(box);
    return adapters::makeFont(getKernelTheme(), textRole, getScale());
}

//==============================================================================

UiDropdown::UiDropdown(const UiThemeResolved& themeIn, UiDropdownRole roleIn)
    : lnf(themeIn, roleIn)
{
    setLookAndFeel(&lnf);
}

UiDropdown::~UiDropdown()
{
    setLookAndFeel(nullptr);
}

void UiDropdown::setTheme(const UiThemeResolved& newTheme)
{
    // LnF is the canonical theme owner; forward to it.
    lnf.setTheme(newTheme);
    repaint();
}

void UiDropdown::setRole(UiDropdownRole newRole)
{
    lnf.setRole(newRole);
    repaint();
}

void UiDropdown::setScale(float newScale)
{
    lnf.setScale(newScale);
    repaint();
}

} // namespace bws::ui
