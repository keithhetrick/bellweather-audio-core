// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

/**
 * @file WeatherTabGroup.h
 * @brief Radio-grouped tab buttons with weather-themed brass styling
 *
 * DESIGN GOALS:
 * - Mutually exclusive tabs (radio button behavior)
 * - Brass gold active state, dark gray inactive
 * - Golden ratio spacing (φ micro = 4px gap)
 * - Horizontal or vertical orientation
 * - Callback on tab selection
 *
 * USAGE:
 * @code
 * auto tabs = std::make_unique<WeatherTabGroup>();
 * tabs->addTab("Balance");
 * tabs->addTab("Width");
 * tabs->onTabChanged = [](int index) {
 *     // Switch parameter mode
 * };
 * tabs->setSelectedTab(0);
 * @endcode
 */

#include <juce_gui_basics/juce_gui_basics.h>
#include <bw_ui/foundation/UiTheme.h>
#include <vector>
#include <functional>

namespace bws::weather
{

/**
 * @class WeatherTabGroup
 * @brief Radio-grouped tab buttons with weather-themed brass styling
 *
 * FEATURES:
 * - Mutually exclusive tabs (radio button behavior)
 * - Brass gold active state, dark gray inactive
 * - Golden ratio spacing (φ micro = 4px gap)
 * - Horizontal or vertical orientation
 * - Callback on tab selection
 */
class WeatherTabGroup : public juce::Component
{
public:
    //==========================================================================
    // CONSTRUCTION
    //==========================================================================

    /**
     * @brief Orientation for tab layout
     */
    enum class Orientation
    {
        Horizontal, ///< Left-to-right layout (default)
        Vertical    ///< Top-to-bottom layout
    };

    /**
     * @brief Constructor
     * @param orientation Tab layout direction
     */
    explicit WeatherTabGroup(Orientation orientation = Orientation::Horizontal);

    ~WeatherTabGroup() override = default;

    //==========================================================================
    // TAB MANAGEMENT
    //==========================================================================

    /**
     * @brief Add a tab button
     * @param label Tab button text
     * @return Index of added tab
     */
    int addTab(const juce::String& label);

    /**
     * @brief Remove all tabs
     */
    void clearTabs();

    /**
     * @brief Get number of tabs
     */
    int getTabCount() const { return static_cast<int>(tabLabels_.size()); }

    //==========================================================================
    // SELECTION
    //==========================================================================

    /**
     * @brief Set active tab by index
     * @param index Tab index (0-based)
     * @param notify If true, triggers onTabChanged callback
     */
    void setSelectedTab(int index, bool notify = false);

    /**
     * @brief Get currently selected tab index
     * @return Active tab index, or -1 if none selected
     */
    int getSelectedTab() const { return selectedIndex_; }

    //==========================================================================
    // SIZING
    //==========================================================================

    /**
     * @brief Set tab button dimensions
     * @param width Button width (default: 54px)
     * @param height Button height (default: 18px)
     */
    void setTabSize(int width, int height);

    /**
     * @brief Set gap between tabs
     * @param gap Gap in pixels (default: 4px = φ micro)
     */
    void setTabGap(int gap);

    /**
     * @brief Calculate ideal size for current tab count
     * @return Minimum size to fit all tabs
     */
    juce::Rectangle<int> getIdealBounds() const;

    //==========================================================================
    // CALLBACKS
    //==========================================================================

    /**
     * @brief Called when tab selection changes
     * @param index New selected tab index
     */
    std::function<void(int index)> onTabChanged;

    /** Apply resolved theme (colors, spacing, typography). */
    void setTheme(const bws::ui::UiThemeResolved& t)
    {
        theme_ = &t;
        repaint();
    }

    void setScale(float scale);

    juce::Font paintFontForTesting() const;

    //==========================================================================
    // juce::Component overrides
    //==========================================================================

    void resized() override;
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    bool keyPressed(const juce::KeyPress& key) override;

private:
    //==========================================================================
    // INTERNAL METHODS
    //==========================================================================

    int getTabIndexAt(juce::Point<int> pos) const;
    void handleTabClick(int index, bool notify);
    std::vector<juce::Rectangle<float>> computeTabLayout() const;
    juce::Font tabLabelFont() const;

    //==========================================================================
    // MEMBER VARIABLES
    //==========================================================================

    std::vector<juce::String> tabLabels_;
    Orientation orientation_;
    int selectedIndex_ = -1;
    int pressedIndex_ = -1;
    int hoveredIndex_ = -1;

    // Sizing
    int tabWidth_ = 54;  // Default from Barometer
    int tabHeight_ = 18; // Default from Barometer
    int tabGap_ = 4;     // φ micro spacing

    const bws::ui::UiThemeResolved* theme_ = nullptr;
    float scale_ = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WeatherTabGroup)
};

} // namespace bws::weather
