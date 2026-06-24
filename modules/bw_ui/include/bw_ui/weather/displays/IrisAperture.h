// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

/**
 * @file IrisAperture.h
 * @brief Brass camera iris aperture component for Weather Instrument Design System
 *
 * DESIGN GOALS:
 * - Brass camera iris that opens/closes to reveal content
 * - 8 animated brass blades (pure Path drawing)
 * - Smooth spring animation (200ms)
 * - Click anywhere to toggle open/close
 * - Content fades in as iris opens
 *
 * USAGE:
 * @code
 * auto iris = std::make_unique<IrisAperture>();
 * iris->setContent(precisionGrid.get());
 * iris->setOpen(false); // Start closed
 * @endcode
 */

#include "bw_ui/foundation/UiTheme.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <memory>

namespace bws::weather
{

/**
 * @class IrisAperture
 * @brief Brass camera iris that opens/closes to reveal content
 *
 * FEATURES:
 * - 8 animated brass blades (procedural Path drawing)
 * - Spring-based animation for smooth open/close transitions
 * - Content component management (non-owning reference)
 * - Content fades in as iris opens past 50%
 * - Callback when open state changes
 * - HiDPI scale factor support
 */
class IrisAperture
    : public juce::Component
    , private juce::Timer
{
public:
    //==========================================================================
    // CONSTRUCTION
    //==========================================================================

    IrisAperture();
    ~IrisAperture() override;

    //==========================================================================
    // CONFIGURATION
    //==========================================================================

    /**
     * @brief Set content component shown when iris is open
     * @param content Component to display (iris does NOT take ownership)
     */
    void setContent(juce::Component* content);

    /**
     * @brief Get the content component (if any)
     * @return Pointer to content component, or nullptr
     */
    juce::Component* getContent() const { return content_; }

    /**
     * @brief Open or close the iris
     * @param open True to open, false to close
     * @param animate True to animate transition (default: true)
     */
    void setOpen(bool open, bool animate = true);

    /**
     * @brief Check if iris is open
     * @return True if open
     */
    bool isOpen() const { return open_; }

    /**
     * @brief Set iris closed diameter (default 60px)
     * @param diameter Diameter when closed
     */
    void setClosedDiameter(int diameter) { closedDiameter_ = diameter; }

    /**
     * @brief Get closed diameter
     * @return Diameter when closed
     */
    int getClosedDiameter() const { return closedDiameter_; }

    /**
     * @brief Set iris open diameter (default 400px)
     * @param diameter Diameter when open
     */
    void setOpenDiameter(int diameter) { openDiameter_ = diameter; }

    /**
     * @brief Get open diameter
     * @return Diameter when open
     */
    int getOpenDiameter() const { return openDiameter_; }

    /**
     * @brief Set scale factor for HiDPI support
     * @param scale Scale factor (1.0 = 100%)
     */
    void setScale(float scale);

    /**
     * @brief Get current scale factor
     * @return Current scale factor
     */
    float getScale() const { return scale_; }

    /**
     * @brief Set drop zone active state (for drag-to-dock visual feedback)
     * @param active True when detached window is in drop zone
     */
    void setDropZoneActive(bool active);

    /**
     * @brief Check if drop zone is currently active
     * @return True if drop zone visual feedback is showing
     */
    bool isDropZoneActive() const { return dropZoneActive_; }

    /**
     * @brief Get current animation progress
     * @return Progress from 0.0 (closed) to 1.0 (open)
     */
    float getOpenProgress() const { return openProgress_; }

    //==========================================================================
    // CALLBACKS
    //==========================================================================

    /**
     * @brief Callback when open state changes
     * @note Called after animation completes (or immediately if no animation)
     */
    std::function<void(bool open)> onOpenChanged;

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
    void mouseMove(const juce::MouseEvent& e) override;
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

    /**
     * @brief Create path for a single iris blade
     * @param bladeIndex Index of blade (0 to NUM_BLADES-1)
     * @param currentRadius Current iris radius
     * @param bladeRotation Current blade rotation angle in degrees
     * @return Path for the blade
     */
    juce::Path createBladePath(int bladeIndex, float currentRadius, float bladeRotation);

    /**
     * @brief Draw the iris blades
     * @param g Graphics context
     * @param centerX Center X position
     * @param centerY Center Y position
     * @param currentRadius Current iris radius
     */
    void drawBlades(juce::Graphics& g, float centerX, float centerY, float currentRadius);

    /**
     * @brief Draw inner decorative ring
     * @param g Graphics context
     * @param centerX Center X position
     * @param centerY Center Y position
     * @param radius Ring radius
     */
    void drawInnerRing(juce::Graphics& g, float centerX, float centerY, float radius);

    //==========================================================================
    // ANIMATION
    //==========================================================================

    void updateContentVisibility();

    //==========================================================================
    // MEMBER VARIABLES
    //==========================================================================

    // Content
    juce::Component* content_ = nullptr;

    // State
    bool open_ = false;
    int closedDiameter_ = 60;
    int openDiameter_ = 400;
    float scale_ = 1.0f;

    // Theme
    void setTheme(const bws::ui::UiThemeResolved& t) { theme_ = &t; }
    const bws::ui::UiThemeResolved* theme_ = nullptr;

    // Animation
    float openProgress_ = 0.0f; // 0.0 = closed, 1.0 = open
    float targetProgress_ = 0.0f;
    bool isAnimating_ = false;

    // Interaction
    bool isHovering_ = false;
    bool isHoveringIris_ = false; // True only when cursor is within iris circle
    float hoverTextAlpha_ = 0.0f; // Animated fade for hover text (0.0 to 1.0)

    // Drop zone feedback (drag-to-dock)
    bool dropZoneActive_ = false;
    float dropZoneGlowPhase_ = 0.0f;

    // Constants
    static constexpr int NUM_BLADES = 8;
    static constexpr float ANIMATION_SPEED = 0.12f;      // Spring constant (higher = faster)
    static constexpr float ANIMATION_THRESHOLD = 0.001f; // When to stop animating

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IrisAperture)
};

} // namespace bws::weather
