// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

/**
 * =============================================================================
 * WeatherLampButton.h - LED Lamp Style Toggle Button
 * =============================================================================
 *
 * Bellweather Studios - Weather Instrument Design System
 *
 * LED lamp style toggle button for scientific instrument aesthetic.
 * Visual: Rounded rectangle housing with circular LED indicator.
 * States: Lit (on) = accent glow, Dim (off) = dark with subtle border.
 * Label: Positioned below or inside per setLabelPosition().
 *
 * Usage:
 *   WeatherLampButton trueCeilButton;
 *   trueCeilButton.setLabel("TRUE CEIL");
 *   trueCeilButton.setLampColour(weatherColors.accent);  // Brass gold when on
 *   addAndMakeVisible(trueCeilButton);
 *   attachment = std::make_unique<ButtonAttachment>(apvts, "trueCeil", trueCeilButton);
 *
 * =============================================================================
 */

#include <juce_gui_basics/juce_gui_basics.h>
#include <bw_ui/foundation/UiTheme.h>

namespace bws::weather
{

/**
 * @brief LED lamp style toggle button for scientific instrument aesthetic.
 *
 * Visual: Rounded rectangle housing with circular LED indicator.
 * States: Lit (on) = accent glow, Dim (off) = dark with subtle border.
 * Label: Positioned below or inside per setLabelPosition().
 */
class WeatherLampButton : public juce::ToggleButton
{
public:
    /** Label position relative to lamp */
    enum class LabelPosition
    {
        Below,
        Inside
    };

    /**
     * @brief Construct a lamp button with optional label.
     * @param label Button label text (displayed below or beside lamp)
     */
    explicit WeatherLampButton(const juce::String& label = {});
    ~WeatherLampButton() override = default;

    /**
     * @brief Set the button label text.
     * @param text Label to display
     */
    void setLabel(const juce::String& text);

    /**
     * @brief Set label position relative to lamp.
     * @param pos LabelPosition::Below (lamp on top) or LabelPosition::Inside (lamp left)
     */
    void setLabelPosition(LabelPosition pos);

    /**
     * @brief Set LED color when active (on state).
     * @param onColour Color for lit lamp
     */
    void setLampColour(juce::Colour onColour);

    /**
     * @brief Set scale factor for HiDPI rendering.
     * @param scale Scale multiplier (1.0 = 100%)
     */
    void setScaleFactor(float scale);

    /** Apply resolved theme (colors, spacing, typography). */
    void setTheme(const bws::ui::UiThemeResolved& t)
    {
        theme_ = &t;
        lampOnColour_ = t.weatherColors.accent;
        repaint();
    }

    // juce::Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    /**
     * @brief Paint the LED lamp indicator.
     * @param g Graphics context
     * @param lampBounds Lamp area bounds
     * @param isOn Whether lamp is lit (on state)
     */
    void paintLamp(juce::Graphics& g, juce::Rectangle<float> lampBounds, bool isOn);

    juce::String labelText_;
    LabelPosition labelPos_ = LabelPosition::Below;
    juce::Colour lampOnColour_ {0xFFD4AF37}; // Default brass gold
    float scaleFactor_ = 1.0f;

    const bws::ui::UiThemeResolved* theme_ = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WeatherLampButton)
};

} // namespace bws::weather
