// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/Components/UiControlTile.h"
#include "bw_ui/Components/UiKnob.h"
#include "bw_ui/Components/UiLabeledSlider.h"
#include "bw_ui/kernel/Knob.h"
#include <cmath>

namespace bws::ui
{
namespace
{
UiReadoutRole resolveTileReadoutRole(UiControlTile::ControlType type, UiKnobRole knobRole, UiReadoutRole requested)
{
    if (type != UiControlTile::ControlType::Knob)
        return requested;

    const auto model = kernel::resolveKnobModel(knobRole);
    if (requested == UiReadoutRole::Standard &&
        model.readoutOwnership == kernel::KnobReadoutOwnership::ExternalCompanion)
    {
        return model.companionReadoutRole;
    }

    return requested;
}
} // namespace

UiControlTile::UiControlTile(ControlType typeIn, juce::Component& controlIn, const UiThemeResolved& themeIn,
                             UiKnobRole knobRoleIn, UiReadoutRole readoutRoleIn)
    : type(typeIn)
    , control(&controlIn)
    , theme(themeIn)
    , kernelTheme(adapters::makeKernelThemeSnapshot(themeIn))
    , knobRole(knobRoleIn)
    , readoutRole(resolveTileReadoutRole(typeIn, knobRoleIn, readoutRoleIn))
    , label(UiTypographyRole::SectionLabel, themeIn)
{
    refreshPadding();
    label.setKernelTheme(kernelTheme);
    label.setTextRole(kernel::TextRole::SectionLabel);
    label.setUseSecondaryColour(true);
    label.setVisible(hasLabel);
    addAndMakeVisible(label);
    addAndMakeVisible(controlIn);
    applyThemeToControl();
}

void UiControlTile::setTheme(const UiThemeResolved& newTheme)
{
    theme = newTheme;
    kernelTheme = adapters::makeKernelThemeSnapshot(newTheme);
    label.setTheme(newTheme);
    label.setKernelTheme(kernelTheme);
    refreshPadding();
    applyThemeToControl();
    repaint();
}

void UiControlTile::setScale(float newScale)
{
    scale = newScale;
    label.setScale(newScale);
    refreshPadding();
    applyScaleToControl();
    resized();
}

void UiControlTile::setLabelText(const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
    hasLabel = text.isNotEmpty();
    label.setVisible(hasLabel);
    resized();
}

void UiControlTile::refreshPadding()
{
    paddingPx = (int)std::round(kernelTheme.spacing.sm * scale);
}

void UiControlTile::applyScaleToControl()
{
    if (auto* knob = dynamic_cast<UiKnob*>(control))
        knob->setScale(scale);
    else if (auto* slider = dynamic_cast<UiLabeledSlider*>(control))
        slider->setScale(scale);
}

void UiControlTile::applyThemeToControl()
{
    if (auto* knob = dynamic_cast<UiKnob*>(control))
        knob->setTheme(theme);
    else if (auto* slider = dynamic_cast<UiLabeledSlider*>(control))
        slider->setTheme(theme);
}

int UiControlTile::preferredKnobWidth(UiKnobRole role, float scale, const UiThemeResolved& theme)
{
    return editor::knobInteractiveSize(role, scale, theme);
}

int UiControlTile::preferredKnobHeight(UiKnobRole role, float scale, const UiThemeResolved& theme,
                                       UiReadoutRole readoutRole)
{
    const auto kernelTheme = adapters::makeKernelThemeSnapshot(theme);
    return editor::knobStackLayout(role, scale, theme, kernelTheme, readoutRole, true).totalHeight;
}

int UiControlTile::preferredSliderHeight(float scale, const UiThemeResolved& theme, UiReadoutRole readoutRole)
{
    const auto kernelTheme = adapters::makeKernelThemeSnapshot(theme);
    return editor::labeledSliderStackLayout(scale, theme, kernelTheme, readoutRole).totalHeight;
}

void UiControlTile::resized()
{
    auto bounds = getLocalBounds().reduced(paddingPx);

    int labelHeight = 0;
    if (hasLabel)
    {
        labelHeight = adapters::sectionTextHeight(kernelTheme, kernel::TextRole::SectionLabel, scale);
        auto labelArea = bounds.removeFromTop(labelHeight);
        label.setBounds(labelArea);
        int gap = adapters::sectionGap(kernelTheme, scale);
        if (type == ControlType::Knob)
            gap = (int)std::round(UiKnobs::metrics(knobRole, scale, theme).labelPadding);
        bounds.removeFromTop(gap);
    }

    const int desiredW =
        (type == ControlType::Knob) ? preferredKnobWidth(knobRole, scale, theme) : juce::jmax(0, bounds.getWidth());
    const int desiredH = (type == ControlType::Knob) ? preferredKnobHeight(knobRole, scale, theme, readoutRole)
                                                     : preferredSliderHeight(scale, theme, readoutRole);

    auto controlArea = editor::fitStackInTile(bounds, desiredW, desiredH, paddingPx);
    if (control != nullptr)
        control->setBounds(controlArea);

    juce::ignoreUnused(labelHeight);
}

} // namespace bws::ui
