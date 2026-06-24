// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/weather/controls/WeatherCompactAction.h"
#include "bw_ui/adapters/UiThemeKernelAdapter.h"

namespace bws::weather
{
namespace
{
const bws::ui::UiThemeResolved& defaultTheme()
{
    static const auto kDefault = bws::ui::resolveTheme(bws::ui::defaultUiThemeBase(), {});
    return kDefault;
}

control_surface::ControlVisualState makeState(const juce::Button& button, bool isMouseOver, bool isButtonDown)
{
    return {button.isEnabled(), isMouseOver, isButtonDown, button.getToggleState(), button.hasKeyboardFocus(true)};
}

juce::Font makeCompactFont(const bws::ui::UiThemeResolved& theme, float scale)
{
    const auto kernelTheme = bws::ui::adapters::makeKernelThemeSnapshot(theme);
    return bws::ui::adapters::makeFont(kernelTheme, bws::ui::kernel::TextRole::ControlText, scale);
}

void paintLabel(juce::Graphics& g, const juce::Rectangle<int>& bounds, const juce::String& label,
                const bws::ui::UiThemeResolved& theme, const control_surface::CompactActionPalette& palette,
                float scale)
{
    const auto kernelTheme = bws::ui::adapters::makeKernelThemeSnapshot(theme);
    g.setFont(makeCompactFont(theme, scale));
    g.setColour(palette.text);
    g.drawFittedText(bws::ui::adapters::applyTextCasing(kernelTheme, bws::ui::kernel::TextRole::ControlText, label),
                     bounds, juce::Justification::centred, 1);
}

// Cased-text advance + 2x the padding token.
int compactActionPreferredWidthPx(const bws::ui::UiThemeResolved& theme, control_surface::CompactActionWidth widthKind,
                                  const juce::String& label, float scale)
{
    const auto kernelTheme = bws::ui::adapters::makeKernelThemeSnapshot(theme);
    const auto cased = bws::ui::adapters::applyTextCasing(kernelTheme, bws::ui::kernel::TextRole::ControlText, label);
    const float advance = juce::GlyphArrangement::getStringWidth(makeCompactFont(theme, scale), cased);
    const float pad = control_surface::resolveCompactActionMetrics(theme, scale, widthKind).horizontalPadding;
    return juce::roundToInt(advance + (2.0f * pad));
}
} // namespace

TogglePill::TogglePill(const juce::String& label, control_surface::CompactActionWidth widthKind)
    : juce::Button(label)
    , widthKind_(widthKind)
{
    setButtonText(label);
    setTriggeredOnMouseDown(false);
    setClickingTogglesState(true);
    setWantsKeyboardFocus(true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void TogglePill::setTheme(const bws::ui::UiThemeResolved& theme)
{
    theme_ = &theme;
    repaint();
}

void TogglePill::setScaleFactor(float scale)
{
    scaleFactor_ = scale;
    repaint();
}

void TogglePill::setLocked(bool locked)
{
    locked_ = locked;
    setAlpha(locked ? 0.55f : 1.0f);
    repaint();
}

void TogglePill::setWidthKind(control_surface::CompactActionWidth widthKind)
{
    widthKind_ = widthKind;
    repaint();
}

void TogglePill::paintButton(juce::Graphics& g, bool isMouseOver, bool isButtonDown)
{
    const auto& theme = resolvedTheme();
    auto metrics = control_surface::resolveCompactActionMetrics(theme, scaleFactor_, widthKind_);
    const auto state = makeState(*this, isMouseOver, isButtonDown);
    const auto palette = control_surface::resolveCompactActionPalette(
        theme, {state.enabled, state.hovered, state.pressed, getToggleState(), state.focused},
        control_surface::CompactActionKind::TogglePill);

    const auto bounds = getLocalBounds().toFloat();
    control_surface::paintCompactAction(g, bounds, metrics, palette);
    paintLabel(g, getLocalBounds(), getButtonText(), theme, palette, scaleFactor_);
    control_surface::paintCompactFocusRing(g, bounds, metrics, palette);
}

void TogglePill::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    if (locked_ || !isEnabled())
        return;

    if (std::abs(wheel.deltaY) > 0.1f)
        setToggleState(!getToggleState(), juce::sendNotificationSync);
}

void TogglePill::mouseDown(const juce::MouseEvent& e)
{
    if (locked_ || !isEnabled())
        return;

    juce::Button::mouseDown(e);
}

void TogglePill::mouseUp(const juce::MouseEvent& e)
{
    if (locked_ || !isEnabled())
        return;

    juce::Button::mouseUp(e);
}

bool TogglePill::keyPressed(const juce::KeyPress& key)
{
    if (locked_ || !isEnabled())
        return false;

    if (key == juce::KeyPress::spaceKey || key == juce::KeyPress::returnKey)
    {
        triggerClick();
        return true;
    }
    return false;
}

const bws::ui::UiThemeResolved& TogglePill::resolvedTheme() const
{
    return theme_ ? *theme_ : defaultTheme();
}

int TogglePill::preferredWidth() const
{
    jassert(theme_ != nullptr);
    return compactActionPreferredWidthPx(resolvedTheme(), widthKind_, getButtonText(), scaleFactor_);
}

ActionPill::ActionPill(const juce::String& label, control_surface::CompactActionWidth widthKind)
    : juce::Button(label)
    , widthKind_(widthKind)
{
    setButtonText(label);
    setTriggeredOnMouseDown(false);
    setWantsKeyboardFocus(true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void ActionPill::setTheme(const bws::ui::UiThemeResolved& theme)
{
    theme_ = &theme;
    repaint();
}

void ActionPill::setScaleFactor(float scale)
{
    scaleFactor_ = scale;
    repaint();
}

void ActionPill::setWidthKind(control_surface::CompactActionWidth widthKind)
{
    widthKind_ = widthKind;
    repaint();
}

void ActionPill::paintButton(juce::Graphics& g, bool isMouseOver, bool isButtonDown)
{
    const auto& theme = resolvedTheme();
    const auto metrics = control_surface::resolveCompactActionMetrics(theme, scaleFactor_, widthKind_);
    const auto palette = control_surface::resolveCompactActionPalette(
        theme, makeState(*this, isMouseOver, isButtonDown), control_surface::CompactActionKind::ActionPill);
    const auto bounds = getLocalBounds().toFloat();

    control_surface::paintCompactAction(g, bounds, metrics, palette);
    paintLabel(g, getLocalBounds(), getButtonText(), theme, palette, scaleFactor_);
    control_surface::paintCompactFocusRing(g, bounds, metrics, palette);
}

bool ActionPill::keyPressed(const juce::KeyPress& key)
{
    if (!isEnabled())
        return false;

    if (key == juce::KeyPress::spaceKey || key == juce::KeyPress::returnKey)
    {
        triggerClick();
        return true;
    }
    return false;
}

const bws::ui::UiThemeResolved& ActionPill::resolvedTheme() const
{
    return theme_ ? *theme_ : defaultTheme();
}

int ActionPill::preferredWidth() const
{
    jassert(theme_ != nullptr);
    return compactActionPreferredWidthPx(resolvedTheme(), widthKind_, getButtonText(), scaleFactor_);
}

UpdateToken::UpdateToken(const juce::String& label)
    : ActionPill(label, control_surface::CompactActionWidth::Narrow)
{}

void UpdateToken::setProductName(const juce::String& productName)
{
    productName_ = productName;
    refreshTooltip();
}

void UpdateToken::setVersionString(const juce::String& version)
{
    versionString_ = version;
    setButtonText(version.isEmpty() ? juce::String {} : "v" + version);
    refreshTooltip();
}

void UpdateToken::clearVersion()
{
    versionString_.clear();
    setButtonText({});
    setTooltip({});
}

void UpdateToken::refreshTooltip()
{
    if (versionString_.isEmpty())
    {
        setTooltip({});
        return;
    }

    const auto product = productName_.isNotEmpty() ? productName_ : juce::String("Update");
    setTooltip(product + " " + versionString_ + " is available. Click to update.");
}

UtilityAction::UtilityAction(const juce::String& label)
    : juce::Button(label)
{
    setButtonText(label);
    setTriggeredOnMouseDown(false);
    setWantsKeyboardFocus(true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void UtilityAction::setTheme(const bws::ui::UiThemeResolved& theme)
{
    theme_ = &theme;
    repaint();
}

void UtilityAction::setScaleFactor(float scale)
{
    scaleFactor_ = scale;
    repaint();
}

void UtilityAction::paintButton(juce::Graphics& g, bool isMouseOver, bool isButtonDown)
{
    const auto& theme = resolvedTheme();
    const auto metrics = control_surface::resolveCompactActionMetrics(theme, scaleFactor_);
    const auto palette = control_surface::resolveCompactActionPalette(
        theme, makeState(*this, isMouseOver, isButtonDown), control_surface::CompactActionKind::UtilityAction);
    const auto bounds = getLocalBounds().toFloat();

    control_surface::paintCompactAction(g, bounds, metrics, palette);
    paintLabel(g, getLocalBounds(), getButtonText(), theme, palette, scaleFactor_);
    control_surface::paintCompactFocusRing(g, bounds, metrics, palette);
}

bool UtilityAction::keyPressed(const juce::KeyPress& key)
{
    if (!isEnabled())
        return false;

    if (key == juce::KeyPress::spaceKey || key == juce::KeyPress::returnKey)
    {
        triggerClick();
        return true;
    }
    return false;
}

const bws::ui::UiThemeResolved& UtilityAction::resolvedTheme() const
{
    return theme_ ? *theme_ : defaultTheme();
}

} // namespace bws::weather
