// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/Components/UiToggle.h"
#include "bw_ui/generated/BwTokens.h"

namespace bws::ui
{

UiToggle::UiToggle(const UiThemeResolved& themeIn, Size sizeIn)
    : theme(themeIn)
    , kernelTheme(adapters::makeKernelThemeSnapshot(themeIn))
    , label(UiTypographyRole::ControlText, themeIn)
    , size(sizeIn)
{
    setWantsKeyboardFocus(true);
    setRepaintsOnMouseActivity(false);
    setInterceptsMouseClicks(true, true);
    label.setKernelTheme(kernelTheme);
    label.setTextRole(kernel::TextRole::ControlText);
}

void UiToggle::setTheme(const UiThemeResolved& newTheme)
{
    theme = newTheme;
    kernelTheme = adapters::makeKernelThemeSnapshot(newTheme);
    label.setTheme(newTheme);
    label.setKernelTheme(kernelTheme);
    repaint();
}

void UiToggle::setScale(float newScale)
{
    scale = newScale;
    label.setScale(newScale);
    resized();
}

void UiToggle::setValue(bool newValue)
{
    commitToggleChange(newValue, false);
}

void UiToggle::setOnChange(std::function<void(bool)> handler)
{
    onChange = std::move(handler);
}

void UiToggle::setLabel(const juce::String& text)
{
    labelText = text;
    const bool visible = labelText.isNotEmpty();
    label.setText(labelText, juce::dontSendNotification);
    label.setVisible(visible);
    if (visible && label.getParentComponent() != this)
        addAndMakeVisible(label);
    resized();
}

void UiToggle::setDisabled(bool shouldDisable)
{
    state.availability = shouldDisable ? kernel::AvailabilityState::Disabled : kernel::AvailabilityState::Enabled;
    pointerDown_ = false;
    state = kernel::resolveControlState(state);
    setInterceptsMouseClicks(kernel::isInteractive(state.availability), kernel::isInteractive(state.availability));
    setWantsKeyboardFocus(kernel::allowsFocus(state.availability));
    if (shouldDisable && hasKeyboardFocus(true))
        giveAwayKeyboardFocus();
    label.setUseSecondaryColour(state.availability == kernel::AvailabilityState::Disabled);
    repaint();
}

void UiToggle::setSize(Size newSize)
{
    size = newSize;
    resized();
}

float UiToggle::trackHeight() const
{
    const float base = (size == Size::Default) ? kernelTheme.spacing.md : kernelTheme.spacing.sm;
    return base * scale;
}

float UiToggle::trackWidth() const
{
    return trackHeight() + kernelTheme.spacing.lg * scale;
}

float UiToggle::cornerRadius() const
{
    return trackHeight() * 0.5f;
}

UiToggle::PaintSpec UiToggle::currentSpec() const
{
    PaintSpec spec;
    auto resolvedState = kernel::resolveControlState(state);
    resolvedState.selected = value;
    const bool active = resolvedState.selected;

    if (active)
    {
        spec.track = resolvedState.pressed ? theme.colors.accent0.darker(0.10f)
                                           : (resolvedState.hovered ? theme.colors.accent1 : theme.colors.accent0);
        spec.outline = juce::Colours::transparentBlack;
        spec.thumb = theme.colors.text0;
    }
    else
    {
        namespace opt = bws::tokens::shared::opacity::ui_toggle;
        spec.track = resolvedState.pressed
                         ? theme.colors.outline0.withAlpha(opt::TRACK_OFF_PRESSED)
                         : (resolvedState.hovered ? theme.colors.outline0.withAlpha(opt::TRACK_OFF_HOVER)
                                                  : theme.colors.outline0.withAlpha(opt::TRACK_OFF_IDLE));
        spec.outline = theme.colors.outline0.withAlpha(opt::OUTLINE_ON);
        spec.thumb = theme.colors.bg0.brighter(0.8f);
    }

    if (resolvedState.availability == kernel::AvailabilityState::Disabled)
    {
        namespace opt = bws::tokens::shared::opacity::ui_toggle;
        spec.track = theme.colors.outline0.withAlpha(opt::DISABLED_TRACK);
        spec.outline = theme.colors.outline0.withAlpha(opt::DISABLED_OUTLINE);
        spec.thumb = theme.colors.textDisabled;
    }

    return spec;
}

juce::Rectangle<float> UiToggle::trackBounds() const
{
    const float h = trackHeight();
    const float w = trackWidth();
    auto bounds = getLocalBounds().toFloat();
    const float x = bounds.getX();
    const float y = bounds.getCentreY() - h * 0.5f;
    return {x, y, w, h};
}

juce::Rectangle<float> UiToggle::thumbBounds(const juce::Rectangle<float>& track) const
{
    const float diameter = juce::jmax(1.0f, track.getHeight() - kernelTheme.spacing.xs * 2.0f * scale);
    const float inset = (track.getHeight() - diameter) * 0.5f;
    const float x = value ? (track.getRight() - diameter - inset) : (track.getX() + inset);
    const float y = track.getY() + inset;
    return {x, y, diameter, diameter};
}

void UiToggle::paint(juce::Graphics& g)
{
    const auto spec = currentSpec();
    auto track = trackBounds();

    g.setColour(spec.track);
    g.fillRoundedRectangle(track, cornerRadius());

    if (spec.outline.getAlpha() > 0.0f)
    {
        g.setColour(spec.outline);
        g.drawRoundedRectangle(track, cornerRadius(), theme.surfaces.strokeThin);
    }

    auto thumb = thumbBounds(track);
    g.setColour(spec.thumb);
    g.fillEllipse(thumb);

    if (state.focused)
    {
        const float pad = juce::jmax(theme.surfaces.strokeThin * 2.0f, kernelTheme.spacing.xxs * scale);
        auto focus = track.expanded(pad);
        g.setColour(theme.colors.accent1.withAlpha(bws::tokens::shared::opacity::ui_toggle::FOCUS_RING));
        g.drawRoundedRectangle(focus, cornerRadius() + pad, theme.surfaces.strokeThin);
    }
}

void UiToggle::resized()
{
    auto bounds = getLocalBounds();
    if (label.isVisible())
    {
        const int gap = (int)std::round(kernelTheme.spacing.sm * scale);
        const int trackW = (int)std::ceil(trackWidth());
        auto trackArea = bounds.removeFromLeft(trackW + gap);
        juce::ignoreUnused(trackArea);
        label.setBounds(bounds);
    }
}

void UiToggle::mouseEnter(const juce::MouseEvent&)
{
    if (!kernel::allowsHover(state.availability))
        return;
    updateHoverState(true);
}

void UiToggle::mouseExit(const juce::MouseEvent&)
{
    if (!kernel::allowsHover(state.availability))
        return;
    updateHoverState(false);
    if (!pointerDown_)
        updatePressedState(false);
}

void UiToggle::mouseDown(const juce::MouseEvent& e)
{
    if (!kernel::isInteractive(state.availability) || !e.mods.isLeftButtonDown())
        return;
    grabKeyboardFocus();
    pointerDown_ = true;
    updateHoverState(true);
    updatePressedState(true);
}

void UiToggle::mouseDrag(const juce::MouseEvent& e)
{
    if (!kernel::isInteractive(state.availability) || !pointerDown_)
        return;

    const bool inside = getLocalBounds().contains(e.getPosition());
    updateHoverState(inside);
    updatePressedState(inside);
}

void UiToggle::mouseUp(const juce::MouseEvent& e)
{
    if (!kernel::isInteractive(state.availability) || !pointerDown_)
        return;

    const bool shouldCommit = getLocalBounds().contains(e.getPosition());
    pointerDown_ = false;
    updateHoverState(shouldCommit);
    updatePressedState(false);

    if (shouldCommit)
        commitToggleChange(!value, true);
}

bool UiToggle::keyPressed(const juce::KeyPress& key)
{
    if (!kernel::isInteractive(state.availability))
        return false;
    if (key == juce::KeyPress::spaceKey || key == juce::KeyPress::returnKey)
    {
        commitToggleChange(!value, true);
        return true;
    }
    return false;
}

void UiToggle::focusGained(FocusChangeType)
{
    state.focused = true;
    state = kernel::resolveControlState(state);
    repaint();
}

void UiToggle::focusLost(FocusChangeType)
{
    state.focused = false;
    pointerDown_ = false;
    state.pressed = false;
    state = kernel::resolveControlState(state);
    repaint();
}

void UiToggle::updatePressedState(bool shouldPress)
{
    if (state.pressed == shouldPress)
        return;

    state.pressed = shouldPress;
    state = kernel::resolveControlState(state);
    repaint();
}

void UiToggle::updateHoverState(bool isHovered)
{
    if (state.hovered == isHovered)
        return;

    state.hovered = isHovered;
    state = kernel::resolveControlState(state);
    repaint();
}

void UiToggle::commitToggleChange(bool newValue, bool fromUserInteraction)
{
    if (value == newValue)
        return;

    value = newValue;
    repaint();

    if (fromUserInteraction && onChange)
        onChange(value);
}

} // namespace bws::ui
