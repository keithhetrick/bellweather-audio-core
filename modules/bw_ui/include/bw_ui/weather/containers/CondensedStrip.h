// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

/**
 * @file CondensedStrip.h
 * @brief Condensed strip component for Weather Instrument Design System
 *
 * DESIGN GOALS:
 * - Collapsed state: 40px strip showing all parameter values at glance
 * - Expanded state: Full height showing content component
 * - Smooth animation between states (300ms)
 * - Brass aesthetic consistent with Weather Instrument suite
 *
 * USAGE:
 * @code
 * auto strip = std::make_unique<CondensedStrip>();
 * strip->setContent(myKnobGrid.get());
 * strip->addParameterValue("thresh", "THR", []{ return "-20.0 dB"; });
 * strip->setExpanded(false); // Start collapsed
 * @endcode
 */

#include "bw_ui/foundation/UiTheme.h"
#include "bw_ui/kernel/Theme.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <memory>
#include <vector>

namespace bws::weather
{

/**
 * @class CondensedStrip
 * @brief Condensed strip that shows parameter values and expands to full content
 *
 * Three states:
 * - Collapsed: 40px strip showing all param values (e.g., "T:-20 R:4 A:10ms")
 * - Expanded: Full height showing content component
 * - Animation: Smooth transition between states
 *
 * FEATURES:
 * - Parameter value display in collapsed state
 * - Click to toggle expansion
 * - Smooth animation during transitions
 * - Content component management (non-owning reference)
 * - Callback when expansion state changes
 */
class CondensedStrip
    : public juce::Component
    , private juce::Timer
{
public:
    //==========================================================================
    // CONSTRUCTION
    //==========================================================================

    CondensedStrip();
    ~CondensedStrip() override;

    //==========================================================================
    // CONFIGURATION
    //==========================================================================

    /**
     * @brief Set content component that appears when expanded
     * @param content Component to show (strip does NOT take ownership)
     */
    void setContent(juce::Component* content);

    /**
     * @brief Get the content component (if any)
     * @return Pointer to content component, or nullptr
     */
    juce::Component* getContent() const { return content_; }

    /**
     * @brief Add a parameter to display in collapsed strip
     * @param paramId Unique ID for parameter
     * @param shortName Short display name (e.g., "THR", "RAT")
     * @param valueGetter Function that returns current value string
     */
    void addParameterValue(const juce::String& paramId, const juce::String& shortName,
                           std::function<juce::String()> valueGetter);

    /**
     * @brief Clear all parameter value displays
     */
    void clearParameterValues();

    /**
     * @brief Set expanded/collapsed state
     * @param expanded True to expand, false to collapse
     * @param animate True to animate transition (default: true)
     */
    void setExpanded(bool expanded, bool animate = true);

    /**
     * @brief Check if strip is expanded
     * @return True if expanded
     */
    bool isExpanded() const { return expanded_; }

    /**
     * @brief Set collapsed height (default 40px)
     * @param height Height when collapsed
     */
    void setCollapsedHeight(int height);

    /**
     * @brief Get collapsed height
     * @return Height when collapsed
     */
    int getCollapsedHeight() const { return collapsedHeight_; }

    /**
     * @brief Set expanded height (default 280px)
     * @param height Height when expanded
     */
    void setExpandedHeight(int height);

    /**
     * @brief Get expanded height
     * @return Height when expanded
     */
    int getExpandedHeight() const { return expandedHeight_; }

    /**
     * @brief Get the current animated height
     * @return Current height during animation
     */
    int getCurrentHeight() const;

    /**
     * @brief Set scale factor for HiDPI support
     * @param scale Scale factor (1.0 = 100%)
     */
    void setScale(float scale);

    /**
     * @brief Set the title text (shown in collapsed strip)
     * @param title Title text (e.g., "ADVANCED")
     */
    void setTitle(const juce::String& title);

    /**
     * @brief Set animation duration in milliseconds
     * @param durationMs Animation duration (default: 300ms)
     */
    void setAnimationDuration(int durationMs);

    //==========================================================================
    // CALLBACKS
    //==========================================================================

    /**
     * @brief Callback when expansion state changes
     * @note Called after animation completes (or immediately if no animation)
     */
    std::function<void(bool expanded)> onExpandedChanged;

    /**
     * @brief Callback during animation for each frame
     * @note Useful for parent layout updates during animation
     */
    std::function<void(float progress)> onAnimationProgress;

    //==========================================================================
    // COMPONENT OVERRIDES
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;

private:
    //==========================================================================
    // INTERNAL TYPES
    //==========================================================================

    struct ParameterValue
    {
        juce::String id;
        juce::String shortName;
        std::function<juce::String()> valueGetter;
    };

    //==========================================================================
    // TIMER CALLBACK
    //==========================================================================

    void timerCallback() override;

    //==========================================================================
    // DRAWING HELPERS
    //==========================================================================

    void paintCollapsedStrip(juce::Graphics& g);
    void paintExpandedHeader(juce::Graphics& g);

    //==========================================================================
    // ANIMATION
    //==========================================================================

    void startAnimation(bool expanding);
    void updateAnimation();

    //==========================================================================
    // LAYOUT
    //==========================================================================

    void updateLayout();

    //==========================================================================
    // MEMBER VARIABLES
    //==========================================================================

    // Configuration
    juce::Component* content_ = nullptr;
    std::vector<ParameterValue> parameters_;
    juce::String title_ = "ADVANCED";

    // State
    bool expanded_ = false;
    int collapsedHeight_ = 40;
    int expandedHeight_ = 280;
    float scale_ = 1.0f;

    // Theme
    void setTheme(const bws::ui::UiThemeResolved& t);
    const bws::ui::UiThemeResolved* theme_ = nullptr;
    bws::ui::kernel::ThemeSnapshot kernelTheme_;

    // Animation
    float expansionProgress_ = 0.0f; // 0.0 = collapsed, 1.0 = expanded
    float targetProgress_ = 0.0f;
    int animationDurationMs_ = 300;
    float animationSpeed_ = 0.0f;
    bool isAnimating_ = false;

    // Interaction
    bool isHovering_ = false;

    // Bounds
    juce::Rectangle<int> headerBounds_;
    juce::Rectangle<int> contentBounds_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CondensedStrip)
};

} // namespace bws::weather
