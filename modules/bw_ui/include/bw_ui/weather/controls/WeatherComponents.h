// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

/**
 * =============================================================================
 * WeatherComponents.h - Reusable UI Components for Weather Instrument Suite
 * =============================================================================
 *
 * Bellweather Studios - Weather Instrument Design System
 *
 * Reusable components for the Weather instrument aesthetic, with brass accents
 * and golden ratio proportions.
 *
 * Components:
 *   1. ABToggle - A/B state comparison toggle
 *   2. MouseWheelSlider - Slider with wheel support for fine adjustment
 *   3. MouseWheelToggle - Toggle button with wheel support
 *   4. ValuePopup - Momentary value display on parameter change
 *   5. ScrollablePresetBox - Preset selector with wheel navigation
 *
 * All components are 100% procedural - no external assets required.
 *
 * =============================================================================
 */

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include "bw_ui/adapters/UiThemeKernelAdapter.h"
#include "bw_ui/foundation/UiTheme.h"

namespace bws::weather
{

// =============================================================================
// ABToggle - A/B State Comparison Toggle
// =============================================================================

/**
 * A/B state comparison toggle with brass highlight.
 *
 * Visual: Shows both A and B states with active one highlighted in brass glow.
 *
 * Interactions:
 *   - Click: Toggle between A/B
 *   - Alt+Click: Copy A → B
 *   - Shift+Click: Copy B → A
 *
 * Usage:
 *   ABToggle abToggle;
 *   abToggle.onClick = [&]() { processor.toggleAB(); };
 *   abToggle.onCopyAToB = [&]() { processor.copyAToB(); };
 */
class ABToggle
    : public juce::Component
    , public juce::SettableTooltipClient
{
public:
    ABToggle();
    ~ABToggle() override = default;

    /** Set active state (true = A, false = B) */
    void setActiveState(bool isA);

    /** Check if showing A state */
    bool isShowingA() const { return showingA_; }

    /** Set scale factor for HiDPI rendering */
    void setScaleFactor(float scale);

    /** Inject resolved theme for typography */
    void setTheme(const bws::ui::UiThemeResolved& t) { theme_ = &t; }

    // -------------------------------------------------------------------------
    // Callbacks
    // -------------------------------------------------------------------------

    /** Called on normal click - toggle A/B */
    std::function<void()> onClick;

    /** Called on Alt+click - copy A to B */
    std::function<void()> onCopyAToB;

    /** Called on Shift+click - copy B to A */
    std::function<void()> onCopyBToA;

    // -------------------------------------------------------------------------
    // juce::Component overrides
    // -------------------------------------------------------------------------

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;

private:
    bool showingA_ = true;
    bool isHovering_ = false;
    float scaleFactor_ = 1.0f;
    const bws::ui::UiThemeResolved* theme_ = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ABToggle)
};

// =============================================================================
// MouseWheelSlider - Slider with Mouse Wheel Support
// =============================================================================

/**
 * Slider that responds to mouse wheel for fine adjustments.
 *
 * Interactions:
 *   - Scroll: Adjust value
 *   - Cmd/Ctrl + Scroll: Fine adjustment (0.001 sensitivity)
 *
 * Usage:
 *   MouseWheelSlider slider;
 *   slider.setSliderStyle(juce::Slider::LinearHorizontal);
 */
class MouseWheelSlider : public juce::Slider
{
public:
    MouseWheelSlider() = default;
    explicit MouseWheelSlider(const juce::String& name);
    ~MouseWheelSlider() override = default;

    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MouseWheelSlider)
};

// =============================================================================
// MouseWheelToggle - Toggle Button with Mouse Wheel Support
// =============================================================================

/**
 * Toggle button that responds to mouse wheel.
 *
 * Interactions:
 *   - Scroll up/down: Toggle state
 *
 * Usage:
 *   MouseWheelToggle toggle("Φ");  // Phase symbol
 *   toggle.setClickingTogglesState(true);
 */
class MouseWheelToggle : public juce::TextButton
{
public:
    MouseWheelToggle() = default;
    explicit MouseWheelToggle(const juce::String& text);
    ~MouseWheelToggle() override = default;

    void setLocked(bool locked);
    bool isLocked() const { return locked_; }

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    bool keyPressed(const juce::KeyPress& key) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

private:
    bool locked_ = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MouseWheelToggle)
};

// =============================================================================
// HeroBrassToggle - Circular Brass Toggle Button for Hero Section
// =============================================================================

/**
 * FabFilter-quality circular brass toggle button for hero section controls.
 *
 * Visual:
 *   - Circular brass button with embossed 3D effect
 *   - Brass gradient: `#A89968` → `#8B7F6A`
 *   - Embossed edge: highlight (top-left), shadow (bottom-right)
 *   - Toggle state glow when active
 *   - Hover: brighten 10%
 *   - Adaptive opacity: idle 85%, hover/active 100%
 *
 * Matches the Weather control chrome for visual consistency.
 *
 * Usage:
 *   HeroBrassToggle toggle("AUTO");
 *   toggle.setClickingTogglesState(true);
 */
class HeroBrassToggle : public juce::Button
{
public:
    HeroBrassToggle()
        : juce::Button("")
    {}
    explicit HeroBrassToggle(const juce::String& text);
    ~HeroBrassToggle() override = default;

    void paintButton(juce::Graphics& g, bool isMouseOver, bool isButtonDown) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

    /** Inject resolved theme for typography */
    void setTheme(const bws::ui::UiThemeResolved& t) { theme_ = &t; }

    /** Set idle opacity (0.0-1.0, default 0.85 for adaptive UI) */
    void setIdleOpacity(float opacity)
    {
        idleOpacity_ = opacity;
        repaint();
    }

    /** Set text color for button label (default: light gray 0xFFE0E0E0) */
    void setTextColor(juce::Colour color)
    {
        textColor_ = color;
        repaint();
    }

    /** Set hover opacity (0.0-1.0, default 1.0) */
    void setHoverOpacity(float opacity)
    {
        hoverOpacity_ = opacity;
        repaint();
    }

    /** Get current text color */
    juce::Colour getTextColor() const { return textColor_; }

private:
    float idleOpacity_ = 0.85f;
    float hoverOpacity_ = 1.0f;
    juce::Colour textColor_ {0xFFE0E0E0}; // Light gray default
    const bws::ui::UiThemeResolved* theme_ = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeroBrassToggle)
};

// =============================================================================
// ValuePopup - Momentary Value Display on Parameter Change
// =============================================================================

/**
 * Shows formatted value briefly when parameter changes.
 *
 * Fades in, shows for ~1.5 seconds, then fades out.
 * Used for non-dial controls to provide visual feedback.
 *
 * Visual: Dark rounded rectangle with parameter name and large value.
 *
 * Usage:
 *   ValuePopup popup;
 *   addChildComponent(&popup);
 *   // On parameter change:
 *   popup.showValue("Balance", "L 25", getMouseXYRelative());
 */
class ValuePopup
    : public juce::Component
    , private juce::Timer
{
public:
    ValuePopup();
    ~ValuePopup() override = default;

    /**
     * Show popup with parameter name and formatted value.
     * @param paramName Parameter display name (e.g., "Balance", "Width")
     * @param value Formatted value string (e.g., "L 25", "100%")
     * @param screenPosition Position in parent coordinates (popup appears above)
     */
    void showValue(const juce::String& paramName, const juce::String& value, juce::Point<int> screenPosition);

    /** Hide the popup immediately */
    void hide();

    /** Set scale factor for HiDPI */
    void setScaleFactor(float scale)
    {
        scaleFactor_ = scale;
        repaint();
    }

    /** Inject resolved theme for typography */
    void setTheme(const bws::ui::UiThemeResolved& t) { theme_ = &t; }

    void paint(juce::Graphics& g) override;

private:
    void timerCallback() override;

    juce::String paramName_;
    juce::String value_;
    float alpha_ = 1.0f;
    int tickCount_ = 0;
    float scaleFactor_ = 1.0f;
    const bws::ui::UiThemeResolved* theme_ = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ValuePopup)
};

// =============================================================================
// ScrollablePresetBox - Preset Selector with Mouse Wheel Navigation
// =============================================================================

/**
 * ComboBox that scrolls through presets with mouse wheel.
 *
 * Interactions:
 *   - Scroll up/down: Navigate presets
 *   - Shift + Scroll: Navigate 5 presets at a time
 *
 * Usage:
 *   ScrollablePresetBox presetBox("Presets");
 *   presetBox.onNavigate = [&](int offset) { navigatePreset(offset); };
 */
class ScrollablePresetBox : public juce::ComboBox
{
public:
    ScrollablePresetBox() = default;
    explicit ScrollablePresetBox(const juce::String& name);
    ~ScrollablePresetBox() override = default;

    /**
     * Callback for preset navigation.
     * @param offset Direction and amount: -1 = previous, +1 = next, ±5 with Shift
     */
    std::function<void(int offset)> onNavigate;

    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScrollablePresetBox)
};

} // namespace bws::weather
