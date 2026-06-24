// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

/**
 * =============================================================================
 * WeatherComponents.cpp - Implementation of Weather Instrument UI Components
 * =============================================================================
 *
 * Bellweather Studios - Weather Instrument Design System
 *
 * Reusable Weather instrument controls backed by the shared design tokens.
 *
 * =============================================================================
 */

#include "bw_ui/weather/controls/WeatherComponents.h"
#include <bw_ui/rendering/JuceRenderer.h>
#include "bw_ui/foundation/UiTheme.h"
#include "bw_ui/generated/BwTokens.h"

namespace bws::weather
{

// =============================================================================
// ABToggle Implementation
// =============================================================================

ABToggle::ABToggle()
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void ABToggle::setActiveState(bool isA)
{
    showingA_ = isA;
    repaint();
}

void ABToggle::setScaleFactor(float scale)
{
    scaleFactor_ = scale;
    repaint();
}

void ABToggle::paint(juce::Graphics& g)
{
    static const auto kDefault = bws::ui::resolveTheme(bws::ui::defaultUiThemeBase(), {});
    const auto& t = theme_ ? *theme_ : kDefault;
    const auto kernelTheme = bws::ui::adapters::makeKernelThemeSnapshot(t);

    bws::ui::rendering::JuceRenderer renderer(g);

    const auto bounds = getLocalBounds().toFloat();
    const auto width = bounds.getWidth();
    const auto height = bounds.getHeight();
    const float cornerRadius = t.weatherColors.radiusMd * scaleFactor_;

    // Background
    renderer.setColour(t.weatherColors.bgRaised.getARGB());
    renderer.fillRoundedRect(bounds.getX(), bounds.getY(), width, height, cornerRadius);

    // Border
    const auto borderBounds = bounds.reduced(bws::tokens::shared::geometry::STROKE_HALF_PX);
    renderer.setColour(t.weatherColors.borderLight.getARGB());
    renderer.drawRoundedRect(borderBounds.getX(), borderBounds.getY(), borderBounds.getWidth(),
                             borderBounds.getHeight(), cornerRadius, 1.0f);

    // Split into A and B sections
    const auto halfWidth = width / 2.0f;
    const juce::Rectangle<float> aRect(bounds.getX(), bounds.getY(), halfWidth, height);
    const juce::Rectangle<float> bRect(bounds.getX() + halfWidth, bounds.getY(), halfWidth, height);

    // Highlight active section - per-corner rounded rect stays on raw g
    // (IRenderer's fillRoundedRect is uniform-radius only)
    if (showingA_)
    {
        juce::Path aPath;
        aPath.addRoundedRectangle(aRect.getX(), aRect.getY(), aRect.getWidth(), aRect.getHeight(), cornerRadius,
                                  cornerRadius, true, false, true, false);
        g.setColour(t.weatherColors.glow);
        g.fillPath(aPath);
    }
    else
    {
        juce::Path bPath;
        bPath.addRoundedRectangle(bRect.getX(), bRect.getY(), bRect.getWidth(), bRect.getHeight(), cornerRadius,
                                  cornerRadius, false, true, false, true);
        g.setColour(t.weatherColors.glow);
        g.fillPath(bPath);
    }

    // Hover effect
    if (isHovering_)
    {
        renderer.setColour(juce::Colours::white.withAlpha(bws::tokens::shared::opacity::shadow::SOFT).getARGB());
        renderer.fillRoundedRect(bounds.getX(), bounds.getY(), width, height, cornerRadius);
    }

    // Text stays on raw g (UiTypography returns Inter typeface)
    g.setColour(showingA_ ? juce::Colours::white : t.weatherColors.textDisabled);
    g.setFont(bws::ui::adapters::makeFont(kernelTheme, bws::ui::kernel::TextRole::Tab, scaleFactor_));
    g.drawText("A", aRect, juce::Justification::centred);

    // Divider line
    const float dividerPadding = 4.0f * scaleFactor_;
    renderer.setColour(t.weatherColors.borderLight.getARGB());
    renderer.drawLine(bounds.getX() + halfWidth, bounds.getY() + dividerPadding, bounds.getX() + halfWidth,
                      bounds.getBottom() - dividerPadding, 1.0f);

    // Text: B (stays on raw g)
    g.setColour(showingA_ ? t.weatherColors.textDisabled : juce::Colours::white);
    g.drawText("B", bRect, juce::Justification::centred);
}

void ABToggle::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isAltDown())
    {
        // Alt+Click: Copy A to B
        if (onCopyAToB)
            onCopyAToB();
    }
    else if (e.mods.isShiftDown())
    {
        // Shift+Click: Copy B to A
        if (onCopyBToA)
            onCopyBToA();
    }
    else
    {
        // Normal click: Toggle A/B
        if (onClick)
            onClick();
    }
}

void ABToggle::mouseEnter(const juce::MouseEvent&)
{
    isHovering_ = true;
    repaint();
}

void ABToggle::mouseExit(const juce::MouseEvent&)
{
    isHovering_ = false;
    repaint();
}

// =============================================================================
// MouseWheelSlider Implementation
// =============================================================================

MouseWheelSlider::MouseWheelSlider(const juce::String& name)
    : juce::Slider(name)
{}

void MouseWheelSlider::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    // Fine-tune with Cmd/Ctrl key
    const bool isFineTune = e.mods.isCommandDown() || e.mods.isCtrlDown();
    const float sensitivity = isFineTune ? 0.001f : 0.01f;
    const float delta = wheel.deltaY * sensitivity;

    const double currentNormalized = valueToProportionOfLength(getValue());
    const double newNormalized = juce::jlimit(0.0, 1.0, currentNormalized + static_cast<double>(delta));
    setValue(proportionOfLengthToValue(newNormalized));
}

// =============================================================================
// MouseWheelToggle Implementation
// =============================================================================

MouseWheelToggle::MouseWheelToggle(const juce::String& text)
    : juce::TextButton(text)
{}

void MouseWheelToggle::setLocked(bool locked)
{
    if (locked_ == locked)
        return;
    locked_ = locked;
    setAlpha(locked ? 0.55f : 1.0f);
    repaint();
}

void MouseWheelToggle::mouseDown(const juce::MouseEvent& e)
{
    if (locked_)
        return;
    juce::TextButton::mouseDown(e);
}

void MouseWheelToggle::mouseUp(const juce::MouseEvent& e)
{
    if (locked_)
        return;
    juce::TextButton::mouseUp(e);
}

bool MouseWheelToggle::keyPressed(const juce::KeyPress& key)
{
    if (locked_)
        return false;
    return juce::TextButton::keyPressed(key);
}

void MouseWheelToggle::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    if (locked_)
        return;

    // Scroll up/down toggles the button state
    if (std::abs(wheel.deltaY) > 0.1f)
    {
        setToggleState(!getToggleState(), juce::sendNotificationSync);
    }
}

// =============================================================================
// HeroBrassToggle Implementation - FabFilter-quality circular brass button
// =============================================================================

HeroBrassToggle::HeroBrassToggle(const juce::String& text)
    : juce::Button(text)
{
    setClickingTogglesState(true);
}

void HeroBrassToggle::paintButton(juce::Graphics& g, bool isMouseOver, bool isButtonDown)
{
    static const auto kDefault = bws::ui::resolveTheme(bws::ui::defaultUiThemeBase(), {});
    const auto& t = theme_ ? *theme_ : kDefault;

    bws::ui::rendering::JuceRenderer renderer(g);

    auto bounds = getLocalBounds().toFloat();

    // Adaptive opacity: idle at reduced opacity, wake up on hover/active
    const float opacity = (isMouseOver || getToggleState()) ? 1.0f : idleOpacity_;
    renderer.setOpacity(opacity);

    // Style: Circular brass buttons with embossed 3D effect
    auto center = bounds.getCentre();
    float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f - 1.0f;
    float diam = radius * 2.0f;

    // Brass gradient
    juce::Colour baseTop = t.weatherColors.surfacePrimary;
    juce::Colour baseBottom = t.weatherColors.surfaceDark;

    if (getToggleState())
    {
        baseTop = t.weatherColors.surfaceHighlight;
        baseBottom = t.weatherColors.surfacePrimary;
    }

    // Fill circular button with brass gradient
    auto gradH = renderer.createLinearGradient(center.x, center.y - radius, center.x, center.y + radius);
    renderer.addGradientStop(gradH, 0.0f, baseTop.getARGB());
    renderer.addGradientStop(gradH, 1.0f, baseBottom.getARGB());
    renderer.applyGradient(gradH);
    renderer.fillEllipse(center.x - radius, center.y - radius, diam, diam);
    renderer.destroyGradient(gradH);

    // Embossed 3D effect
    if (isButtonDown)
    {
        // Pressed: inner shadow
        renderer.setColour(juce::Colours::black.withAlpha(bws::tokens::shared::opacity::shadow::DROP).getARGB());
        renderer.drawEllipse(center.x - radius + 0.5f, center.y - radius + 0.5f, diam - 1.0f, diam - 1.0f, 1.0f);
    }
    else
    {
        // Normal: embossed edges - top-left highlight arc
        renderer.setColour(juce::Colours::white.withAlpha(bws::tokens::shared::opacity::shadow::HIGHLIGHT).getARGB());
        renderer.beginPath();
        renderer.arcTo(center.x, center.y, radius - 0.5f, -2.356f, 0.785f);
        renderer.strokePath(1.0f);

        // Bottom-right shadow arc
        renderer.setColour(juce::Colours::black.withAlpha(bws::tokens::shared::opacity::shadow::DROP).getARGB());
        renderer.beginPath();
        renderer.arcTo(center.x, center.y, radius - 0.5f, 0.785f, 4.0f);
        renderer.strokePath(1.0f);
    }

    // Toggle state glow (warm brass inner ring)
    if (getToggleState())
    {
        renderer.setColour(
            t.weatherColors.surfaceHighlight.withAlpha(bws::tokens::shared::opacity::shadow::SURFACE_HIGH).getARGB());
        renderer.drawEllipse(center.x - radius + 2.0f, center.y - radius + 2.0f, (radius - 2.0f) * 2.0f,
                             (radius - 2.0f) * 2.0f, 1.5f);
    }

    // Hover effect
    if (isMouseOver && !isButtonDown)
    {
        renderer.setColour(juce::Colours::white.withAlpha(bws::tokens::shared::opacity::shadow::SUBTLE).getARGB());
        renderer.fillEllipse(center.x - radius, center.y - radius, diam, diam);

        if (!getToggleState())
        {
            renderer.setColour(
                t.weatherColors.surfaceHighlight.withAlpha(bws::tokens::shared::opacity::shadow::SURFACE_SUBTLE)
                    .getARGB());
            renderer.drawEllipse(center.x - radius - 1.0f, center.y - radius - 1.0f, diam + 2.0f, diam + 2.0f, 2.0f);
        }
    }

    // Reset opacity for text
    renderer.setOpacity(1.0f);

    // Button text stays on raw g (UiTypography returns Inter typeface)
    g.setColour(getToggleState() ? t.weatherColors.surfaceHighlight.brighter(0.3f) : textColor_);
    const auto kernelTheme = bws::ui::adapters::makeKernelThemeSnapshot(t);
    g.setFont(bws::ui::adapters::makeFont(kernelTheme, bws::ui::kernel::TextRole::ControlText, 1.0f));
    g.drawText(getButtonText(), bounds, juce::Justification::centred);
}

void HeroBrassToggle::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    // Scroll up/down toggles the button state (same as MouseWheelToggle)
    if (std::abs(wheel.deltaY) > 0.1f)
    {
        setToggleState(!getToggleState(), juce::sendNotificationSync);
    }
}

// =============================================================================
// ValuePopup Implementation
// =============================================================================

ValuePopup::ValuePopup()
{
    setAlwaysOnTop(true);
    setOpaque(false);
}

void ValuePopup::showValue(const juce::String& paramName, const juce::String& value, juce::Point<int> screenPosition)
{
    paramName_ = paramName;
    value_ = value;
    alpha_ = 1.0f;
    tickCount_ = 0;

    const int popupWidth = static_cast<int>(120 * scaleFactor_);
    const int popupHeight = static_cast<int>(50 * scaleFactor_);
    setSize(popupWidth, popupHeight);
    setCentrePosition(screenPosition.x, screenPosition.y - static_cast<int>(60 * scaleFactor_));

    setVisible(true);
    startTimer(50); // Update every 50ms for fade effect
    repaint();
}

void ValuePopup::hide()
{
    stopTimer();
    setVisible(false);
}

void ValuePopup::paint(juce::Graphics& g)
{
    static const auto kDefault = bws::ui::resolveTheme(bws::ui::defaultUiThemeBase(), {});
    const auto& t = theme_ ? *theme_ : kDefault;
    const auto kernelTheme = bws::ui::adapters::makeKernelThemeSnapshot(t);

    bws::ui::rendering::JuceRenderer renderer(g);

    auto bounds = getLocalBounds().toFloat();
    const float cornerRadius = t.weatherColors.radiusLg * scaleFactor_;

    // Semi-transparent dark background with rounded corners
    renderer.setColour(
        t.weatherColors.bgMain.withAlpha(alpha_ * bws::tokens::shared::opacity::shadow::BG_MULT).getARGB());
    renderer.fillRoundedRect(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), cornerRadius);

    // Border
    const auto borderBounds = bounds.reduced(bws::tokens::shared::geometry::STROKE_HALF_PX);
    renderer.setColour(
        t.weatherColors.borderLight.withAlpha(alpha_ * bws::tokens::shared::opacity::shadow::BORDER_MULT).getARGB());
    renderer.drawRoundedRect(borderBounds.getX(), borderBounds.getY(), borderBounds.getWidth(),
                             borderBounds.getHeight(), cornerRadius, 1.0f);

    // Text stays on raw g (UiTypography returns Inter typeface)
    // Parameter name (small, dimmer)
    g.setColour(juce::Colours::white.withAlpha(alpha_ * bws::tokens::shared::opacity::shadow::WHITE_MUTED));
    g.setFont(bws::ui::adapters::makeFont(kernelTheme, bws::ui::kernel::TextRole::ControlText, scaleFactor_));
    g.drawText(paramName_, bounds.removeFromTop(18.0f * scaleFactor_), juce::Justification::centred);

    // Value (large, bright)
    g.setColour(juce::Colours::white.withAlpha(alpha_ * bws::tokens::shared::opacity::shadow::WHITE_STRONG));
    g.setFont(bws::ui::adapters::makeFont(kernelTheme, bws::ui::kernel::TextRole::Readout, scaleFactor_));
    g.drawText(value_, bounds, juce::Justification::centred);
}

void ValuePopup::timerCallback()
{
    tickCount_++;

    // Show for ~1.5 seconds, then start fading
    if (tickCount_ > 30) // 1.5 seconds at 50ms interval
    {
        alpha_ -= 0.1f;
        repaint();

        if (alpha_ <= 0.0f)
        {
            hide();
        }
    }
}

// =============================================================================
// ScrollablePresetBox Implementation
// =============================================================================

ScrollablePresetBox::ScrollablePresetBox(const juce::String& name)
    : juce::ComboBox(name)
{}

void ScrollablePresetBox::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (onNavigate && wheel.deltaY != 0.0f)
    {
        // Determine direction: scroll up = next preset (+1), scroll down = previous (-1)
        // Note: deltaY positive typically means scroll up (varies by platform)
        const int direction = wheel.deltaY > 0.0f ? -1 : 1;
        const int step = e.mods.isShiftDown() ? 5 : 1;

        onNavigate(direction * step);
    }
    else
    {
        juce::ComboBox::mouseWheelMove(e, wheel);
    }
}

} // namespace bws::weather
