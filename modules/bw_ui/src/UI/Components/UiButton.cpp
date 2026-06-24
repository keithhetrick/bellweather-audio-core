// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/Components/UiButton.h"
#include "bw_ui/generated/BwTokens.h"

namespace bws::ui
{

UiButton::UiButton(const UiThemeResolved& themeIn, juce::String labelText, Variant variantIn, Size sizeIn)
    : theme(themeIn)
    , kernelTheme(adapters::makeKernelThemeSnapshot(themeIn))
    , label(std::move(labelText))
    , variant(variantIn)
    , size(sizeIn)
{
    setWantsKeyboardFocus(true);
    setRepaintsOnMouseActivity(false);
    setInterceptsMouseClicks(true, true);
}

void UiButton::setTheme(const UiThemeResolved& newTheme)
{
    theme = newTheme;
    kernelTheme = adapters::makeKernelThemeSnapshot(newTheme);
    repaint();
}

void UiButton::setScale(float newScale)
{
    scale = newScale;
    repaint();
}

void UiButton::setLabel(juce::String newLabel)
{
    label = std::move(newLabel);
    repaint();
}

void UiButton::setVariant(Variant newVariant)
{
    variant = newVariant;
    repaint();
}

void UiButton::setSize(Size newSize)
{
    size = newSize;
    repaint();
}

void UiButton::setDisabled(bool shouldDisable)
{
    state.availability = shouldDisable ? kernel::AvailabilityState::Disabled : kernel::AvailabilityState::Enabled;
    pointerDown_ = false;
    state = kernel::resolveControlState(state);
    setInterceptsMouseClicks(kernel::isInteractive(state.availability), kernel::isInteractive(state.availability));
    setWantsKeyboardFocus(kernel::allowsFocus(state.availability));
    if (shouldDisable && hasKeyboardFocus(true))
        giveAwayKeyboardFocus();
    repaint();
}

void UiButton::setOnClick(std::function<void()> handler)
{
    onClick = std::move(handler);
}

UiButton::PaintSpec UiButton::currentSpec() const
{
    PaintSpec spec;
    const auto resolvedState = kernel::resolveControlState(state);

    const bool isText = variant == Variant::Text;
    const bool isGhost = variant == Variant::Ghost;

    if (variant == Variant::Primary)
    {
        spec.background = resolvedState.pressed ? theme.colors.accent0.darker(0.10f)
                                                : (resolvedState.hovered ? theme.colors.accent1 : theme.colors.accent0);
        spec.outline = juce::Colours::transparentBlack;
        spec.text = theme.colors.text0;
    }
    else if (isGhost)
    {
        namespace opb = bws::tokens::shared::opacity::ui_button;
        const auto base = theme.colors.bg1.withAlpha(opb::GHOST_BASE);
        const auto hover = theme.colors.accent0.withAlpha(opb::GHOST_HOVER);
        const auto active = theme.colors.accent0.withAlpha(opb::GHOST_ACTIVE);
        spec.background = resolvedState.pressed ? active : (resolvedState.hovered ? hover : base);
        spec.outline = theme.colors.outline0.withAlpha(opb::GHOST_OUTLINE);
        spec.text = theme.colors.text0;
    }
    else // Text
    {
        namespace opb = bws::tokens::shared::opacity::ui_button;
        spec.background = resolvedState.pressed
                              ? theme.colors.accent0.withAlpha(opb::TEXT_PRESSED)
                              : (resolvedState.hovered ? theme.colors.accent0.withAlpha(opb::TEXT_HOVER)
                                                       : juce::Colours::transparentBlack);
        spec.outline = juce::Colours::transparentBlack;
        spec.text = theme.colors.accent0;
    }

    if (resolvedState.availability == kernel::AvailabilityState::Disabled)
    {
        namespace opb = bws::tokens::shared::opacity::ui_button;
        spec.background = spec.background.withAlpha(opb::DISABLED_BG);
        spec.outline = spec.outline.withAlpha(opb::DISABLED_OUTLINE);
        spec.text = theme.colors.textDisabled;
    }

    return spec;
}

float UiButton::radius() const
{
    return theme.surfaces.radiusSm * scale;
}

juce::Font UiButton::labelFont() const
{
    return adapters::makeFont(kernelTheme, kernel::TextRole::ControlText, scale);
}

juce::Rectangle<int> UiButton::contentBounds() const
{
    const float padY = (size == Size::Default) ? kernelTheme.spacing.sm : kernelTheme.spacing.xs;
    const float padX = (size == Size::Default) ? kernelTheme.spacing.md : kernelTheme.spacing.sm;
    auto bounds = getLocalBounds();
    bounds.reduce((int)std::round(padX * scale), (int)std::round(padY * scale));
    return bounds;
}

void UiButton::paint(juce::Graphics& g)
{
    const auto spec = currentSpec();
    auto bounds = getLocalBounds().toFloat();

    g.setColour(spec.background);
    g.fillRoundedRectangle(bounds, radius());

    if (spec.outline.getAlpha() > 0)
    {
        g.setColour(spec.outline);
        g.drawRoundedRectangle(bounds, radius(), theme.surfaces.strokeThin);
    }

    if (state.focused)
    {
        const float inset = juce::jmax(1.0f, theme.surfaces.strokeThin * 2.0f);
        auto focusBounds = bounds.reduced(inset);
        g.setColour(theme.colors.accent1.withAlpha(bws::tokens::shared::opacity::ui_button::FOCUS_RING));
        g.drawRoundedRectangle(focusBounds, radius(), theme.surfaces.strokeThin);
    }

    g.setFont(labelFont());
    g.setColour(spec.text);
    g.drawFittedText(adapters::applyTextCasing(kernelTheme, kernel::TextRole::ControlText, label), contentBounds(),
                     juce::Justification::centred, 1);
}

void UiButton::mouseEnter(const juce::MouseEvent&)
{
    if (!kernel::allowsHover(state.availability))
        return;
    updateHoverState(true);
}

void UiButton::mouseExit(const juce::MouseEvent&)
{
    if (!kernel::allowsHover(state.availability))
        return;
    updateHoverState(false);
    if (!pointerDown_)
        updatePressedState(false);
}

void UiButton::mouseDown(const juce::MouseEvent& e)
{
    if (!kernel::isInteractive(state.availability) || !e.mods.isLeftButtonDown())
        return;
    grabKeyboardFocus();
    pointerDown_ = true;
    updateHoverState(true);
    updatePressedState(true);
}

void UiButton::mouseDrag(const juce::MouseEvent& e)
{
    if (!kernel::isInteractive(state.availability) || !pointerDown_)
        return;

    const bool inside = getLocalBounds().contains(e.getPosition());
    updateHoverState(inside);
    updatePressedState(inside);
}

void UiButton::mouseUp(const juce::MouseEvent& e)
{
    if (!kernel::isInteractive(state.availability) || !pointerDown_)
        return;

    const bool shouldCommit = getLocalBounds().contains(e.getPosition());
    pointerDown_ = false;
    updateHoverState(shouldCommit);
    updatePressedState(false);

    if (shouldCommit)
        commitIfPossible();
}

bool UiButton::keyPressed(const juce::KeyPress& key)
{
    if (!kernel::isInteractive(state.availability))
        return false;
    if (key == juce::KeyPress::returnKey || key == juce::KeyPress::spaceKey)
    {
        commitIfPossible();
        return true;
    }
    return false;
}

void UiButton::focusGained(FocusChangeType)
{
    state.focused = true;
    state = kernel::resolveControlState(state);
    repaint();
}

void UiButton::focusLost(FocusChangeType)
{
    state.focused = false;
    pointerDown_ = false;
    state.pressed = false;
    state = kernel::resolveControlState(state);
    repaint();
}

void UiButton::updatePressedState(bool shouldPress)
{
    if (state.pressed == shouldPress)
        return;

    state.pressed = shouldPress;
    state = kernel::resolveControlState(state);
    repaint();
}

void UiButton::updateHoverState(bool isHovered)
{
    if (state.hovered == isHovered)
        return;

    state.hovered = isHovered;
    state = kernel::resolveControlState(state);
    repaint();
}

void UiButton::commitIfPossible()
{
    if (!kernel::isInteractive(state.availability))
        return;

    if (onClick)
        onClick();
}

// Factory methods
std::unique_ptr<UiButton> UiButton::xs(const UiThemeResolved& theme, const juce::String& text, Variant variant)
{
    auto button = std::make_unique<UiButton>(theme, text, variant);
    button->setSizeVariant(ComponentSize::XS);
    return button;
}

std::unique_ptr<UiButton> UiButton::sm(const UiThemeResolved& theme, const juce::String& text, Variant variant)
{
    auto button = std::make_unique<UiButton>(theme, text, variant);
    button->setSizeVariant(ComponentSize::SM);
    return button;
}

std::unique_ptr<UiButton> UiButton::md(const UiThemeResolved& theme, const juce::String& text, Variant variant)
{
    auto button = std::make_unique<UiButton>(theme, text, variant);
    button->setSizeVariant(ComponentSize::MD);
    return button;
}

std::unique_ptr<UiButton> UiButton::lg(const UiThemeResolved& theme, const juce::String& text, Variant variant)
{
    auto button = std::make_unique<UiButton>(theme, text, variant);
    button->setSizeVariant(ComponentSize::LG);
    return button;
}

std::unique_ptr<UiButton> UiButton::xl(const UiThemeResolved& theme, const juce::String& text, Variant variant)
{
    auto button = std::make_unique<UiButton>(theme, text, variant);
    button->setSizeVariant(ComponentSize::XL);
    return button;
}

void UiButton::setSizeVariant(ComponentSize size)
{
    currentSizeVariant = size;
    applySizeVariant(size);
}

void UiButton::applySizeVariant(ComponentSize size)
{
    // Map size enum to actual dimensions from theme
    int width = 100;
    int height = 40;

    switch (size)
    {
    case ComponentSize::XS:
        width = 60;
        height = 24;
        break;
    case ComponentSize::SM:
        width = 80;
        height = 32;
        break;
    case ComponentSize::MD:
        width = 100;
        height = 40;
        break;
    case ComponentSize::LG:
        width = 140;
        height = 48;
        break;
    case ComponentSize::XL:
        width = 180;
        height = 56;
        break;
    }

    Component::setSize(width, height);
}

} // namespace bws::ui
