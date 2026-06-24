// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

/**
 * @file CollapsiblePanel.h
 * @brief Collapsible panel component for Weather Instrument Design System
 *
 * DESIGN GOALS:
 * - Two-panel system support (hero controls + advanced controls)
 * - Smooth expand/collapse animation (300ms)
 * - Brass header aesthetic with arrow indicator
 * - Preference persistence per plugin
 *
 * USAGE:
 * @code
 * auto panel = std::make_unique<CollapsiblePanel>("PRECISION CONTROLS");
 * panel->setContent(createAdvancedControls());
 * panel->setExpanded(false); // Collapsed by default
 * panel->onExpandedChanged = [](bool expanded) {
 *     // Handle resize if needed
 * };
 * addAndMakeVisible(panel.get());
 * @endcode
 */

#include "bw_ui/foundation/UiTheme.h"
#include "bw_ui/adapters/UiThemeKernelAdapter.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <memory>

namespace bws::weather
{

/**
 * @class CollapsiblePanel
 * @brief Expandable/collapsible container for advanced controls
 *
 * FEATURES:
 * - Brass-styled header with title and arrow indicator
 * - Click header to toggle expanded state
 * - Smooth animation during expand/collapse
 * - Content component management (takes ownership)
 * - Callback when expansion state changes
 */
class CollapsiblePanel
    : public juce::Component
    , private juce::Timer
{
public:
    //==========================================================================
    // CONSTRUCTION
    //==========================================================================

    /**
     * @brief Create collapsible panel with title
     * @param title Header text (e.g., "PRECISION CONTROLS")
     */
    explicit CollapsiblePanel(const juce::String& title);

    ~CollapsiblePanel() override;

    //==========================================================================
    // CONFIGURATION
    //==========================================================================

    /**
     * @brief Set content component that appears when expanded
     * @param content Component to show (panel takes ownership)
     */
    void setContent(std::unique_ptr<juce::Component> content);

    /**
     * @brief Get the content component (if any)
     * @return Pointer to content component, or nullptr
     */
    juce::Component* getContent() const { return content_.get(); }

    /**
     * @brief Expand or collapse the panel
     * @param expanded True to expand, false to collapse
     * @param animate True to animate the transition (default: true)
     */
    void setExpanded(bool expanded, bool animate = true);

    /**
     * @brief Check if panel is expanded
     * @return True if expanded
     */
    bool isExpanded() const { return expanded_; }

    /**
     * @brief Set header height
     * @param height Header bar height in pixels
     */
    void setHeaderHeight(int height);

    /**
     * @brief Get current header height
     * @return Header height in pixels
     */
    int getHeaderHeight() const { return headerHeight_; }

    /**
     * @brief Set whether to show expand/collapse arrow
     * @param show True to show arrow indicator
     */
    void setShowArrow(bool show);

    /**
     * @brief Set the title text
     * @param title New title text
     */
    void setTitle(const juce::String& title);

    /**
     * @brief Get the title text
     * @return Current title
     */
    juce::String getTitle() const { return title_; }

    /**
     * @brief Set animation duration in milliseconds
     * @param durationMs Animation duration (default: 300ms)
     */
    void setAnimationDuration(int durationMs);

    /**
     * @brief Set scale factor for HiDPI support
     * @param scale Scale factor (1.0 = 100%)
     */
    void setScale(float scale);

    //==========================================================================
    // SIZE CALCULATION
    //==========================================================================

    /**
     * @brief Get the preferred height when collapsed
     * @return Height needed when collapsed (header only)
     */
    int getCollapsedHeight() const;

    /**
     * @brief Get the preferred height when fully expanded
     * @return Height needed when expanded (header + content)
     */
    int getExpandedHeight() const;

    /**
     * @brief Get the current animated height
     * @return Current height during animation
     */
    int getCurrentHeight() const;

    /**
     * @brief Set the content height (height of content when expanded)
     * @param height Content area height in pixels
     */
    void setContentHeight(int height);

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
    // TIMER CALLBACK
    //==========================================================================

    void timerCallback() override;

    //==========================================================================
    // DRAWING HELPERS
    //==========================================================================

    void paintHeader(juce::Graphics& g);
    void paintArrow(juce::Graphics& g, juce::Rectangle<float> bounds);
    void paintContentBorder(juce::Graphics& g);

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
    juce::String title_;
    std::unique_ptr<juce::Component> content_;
    bool expanded_ = false;
    bool showArrow_ = true;
    int headerHeight_ = 40;
    int contentHeight_ = 200;
    float scale_ = 1.0f;

    // Theme
    void setTheme(const bws::ui::UiThemeResolved& t) { theme_ = &t; }
    const bws::ui::UiThemeResolved* theme_ = nullptr;

    // Animation
    float expansionProgress_ = 0.0f; // 0.0 = collapsed, 1.0 = expanded
    float targetProgress_ = 0.0f;
    int animationDurationMs_ = 300;
    float animationSpeed_ = 0.0f; // Progress per timer tick
    bool isAnimating_ = false;

    // Interaction
    bool isHovering_ = false;

    // Bounds (calculated in resized)
    juce::Rectangle<int> headerBounds_;
    juce::Rectangle<int> contentBounds_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CollapsiblePanel)
};

} // namespace bws::weather
