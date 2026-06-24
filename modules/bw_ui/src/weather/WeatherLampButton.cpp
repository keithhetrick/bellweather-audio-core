// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

/**
 * =============================================================================
 * WeatherLampButton.cpp - LED Lamp Style Toggle Button Implementation
 * =============================================================================
 *
 * Bellweather Studios - Weather Instrument Design System
 *
 * Procedural LED lamp rendering with lit/dim states.
 * Uses Brass design tokens for consistent visual language.
 *
 * =============================================================================
 */

#include "bw_ui/weather/controls/WeatherLampButton.h"
#include "bw_ui/weather/controls/WeatherControlSurface.h"
#include "bw_ui/adapters/UiThemeKernelAdapter.h"
#include "bw_ui/foundation/UiTheme.h"
#include "bw_ui/generated/BwTokens.h"

namespace bws::weather
{

WeatherLampButton::WeatherLampButton(const juce::String& label)
    : labelText_(label)
{
    setClickingTogglesState(true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void WeatherLampButton::setLabel(const juce::String& text)
{
    labelText_ = text;
    repaint();
}

void WeatherLampButton::setLabelPosition(LabelPosition pos)
{
    labelPos_ = pos;
    repaint();
}

void WeatherLampButton::setLampColour(juce::Colour onColour)
{
    lampOnColour_ = onColour;
    repaint();
}

void WeatherLampButton::setScaleFactor(float scale)
{
    scaleFactor_ = scale;
    repaint();
}

void WeatherLampButton::paint(juce::Graphics& g)
{
    static const auto kDefault = bws::ui::resolveTheme(bws::ui::defaultUiThemeBase(), {});
    const auto& t = theme_ ? *theme_ : kDefault;
    const auto kernelTheme = bws::ui::adapters::makeKernelThemeSnapshot(t);

    const auto bounds = getLocalBounds().toFloat();
    const bool isOn = getToggleState();
    const auto metrics = control_surface::resolveSelectorMetrics(t, scaleFactor_);
    const auto state = control_surface::ControlVisualState {isEnabled(), isMouseOver(), isMouseButtonDown(), isOn,
                                                            hasKeyboardFocus(true)};
    const auto palette = control_surface::resolveSelectorPalette(t, state);
    const auto labelFont =
        bws::ui::adapters::makeFont(kernelTheme, bws::ui::kernel::TextRole::ControlText, scaleFactor_);

    control_surface::paintContainer(g, bounds, metrics, palette);

    // Calculate lamp and label areas
    const float lampDiameter = 10.0f * scaleFactor_;
    const float labelHeight = labelFont.getHeight();
    const float padding = 4.0f * scaleFactor_;

    juce::Rectangle<float> lampBounds;
    juce::Rectangle<float> labelBounds;

    if (labelPos_ == LabelPosition::Below)
    {
        // Lamp centered horizontally, top area
        lampBounds = juce::Rectangle<float>(bounds.getCentreX() - lampDiameter / 2,
                                            bounds.getY() + padding + 2.0f * scaleFactor_, lampDiameter, lampDiameter);
        // Label below lamp
        labelBounds = juce::Rectangle<float>(bounds.getX(), lampBounds.getBottom() + 2.0f * scaleFactor_,
                                             bounds.getWidth(), labelHeight + 2.0f * scaleFactor_);
    }
    else // Inside
    {
        // Lamp on left, label on right
        lampBounds = juce::Rectangle<float>(bounds.getX() + padding, bounds.getCentreY() - lampDiameter / 2,
                                            lampDiameter, lampDiameter);
        labelBounds =
            juce::Rectangle<float>(lampBounds.getRight() + padding, bounds.getY(),
                                   bounds.getWidth() - lampBounds.getRight() - padding * 2, bounds.getHeight());
    }

    // Draw LED lamp
    paintLamp(g, lampBounds, isOn);

    // Draw label
    g.setColour(palette.text);
    g.setFont(labelFont);
    g.drawText(labelText_, labelBounds,
               labelPos_ == LabelPosition::Below ? juce::Justification::centredTop : juce::Justification::centredLeft);

    control_surface::paintFocusRing(g, bounds, metrics, palette);
}

void WeatherLampButton::paintLamp(juce::Graphics& g, juce::Rectangle<float> lampBounds, bool isOn)
{
    static const auto kDefault = bws::ui::resolveTheme(bws::ui::defaultUiThemeBase(), {});
    const auto& t = theme_ ? *theme_ : kDefault;

    const auto centre = lampBounds.getCentre();
    const float radius = lampBounds.getWidth() / 2.0f;

    if (isOn)
    {
        // Outer glow (subtle, not aggressive per CONTEXT.md - 0.3 alpha)
        juce::ColourGradient glow(lampOnColour_.withAlpha(bws::tokens::shared::opacity::lamp::GLOW_CENTER), centre.x,
                                  centre.y, lampOnColour_.withAlpha(0.0f), centre.x + radius * 1.8f, centre.y,
                                  true // radial
        );
        g.setGradientFill(glow);
        g.fillEllipse(lampBounds.expanded(radius * 0.5f));

        // Inner lamp (bright)
        g.setColour(lampOnColour_);
        g.fillEllipse(lampBounds);

        // Highlight (top-left specular)
        g.setColour(juce::Colours::white.withAlpha(bws::tokens::shared::opacity::lamp::HIGHLIGHT_CAP));
        g.fillEllipse(lampBounds.reduced(radius * 0.4f).translated(-radius * 0.15f, -radius * 0.15f));
    }
    else
    {
        // Dim lamp (dark with subtle border)
        g.setColour(t.weatherColors.bgInput);
        g.fillEllipse(lampBounds);
        g.setColour(t.weatherColors.borderLight);
        g.drawEllipse(lampBounds.reduced(bws::tokens::shared::geometry::STROKE_HALF_PX), 1.0f);
    }
}

void WeatherLampButton::resized()
{
    // No child components to layout
}

} // namespace bws::weather
