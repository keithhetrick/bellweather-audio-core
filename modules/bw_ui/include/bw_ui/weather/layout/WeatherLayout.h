// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

/**
 * =============================================================================
 * WeatherLayout.h - Golden Ratio Layout Utilities
 * =============================================================================
 *
 * Bellweather Studios - Weather Instrument Design System
 *
 * Layout utilities based on golden ratio (φ = 1.618) for creating
 * visually harmonious plugin interfaces.
 *
 * Key concepts:
 *   - Hero dial positioned at φ ratio of total height
 *   - Spacing using Fibonacci-derived values (8, 13, 21, 34, 55...)
 *   - Plugin tiers with pre-computed dimensions
 *
 * =============================================================================
 */

#include <juce_gui_basics/juce_gui_basics.h>

namespace bws::weather
{

/**
 * Golden Ratio Layout Utilities
 *
 * Provides helper functions and constants for creating layouts
 * that follow the golden ratio proportions.
 */
class WeatherLayout
{
public:
    // =========================================================================
    // Constants
    // =========================================================================

    /** Golden ratio (φ) */
    static constexpr float PHI = 1.618033988749895f;

    /** Inverse golden ratio (1/φ) */
    static constexpr float PHI_INVERSE = 0.618033988749895f;

    /** φ² for larger proportions */
    static constexpr float PHI_SQUARED = 2.618033988749895f;

    /** Base spacing unit (8px) */
    static constexpr float SPACING_BASE = 8.0f;

    // =========================================================================
    // Instrument Plugin (1-4 controls, dial-dominant)
    // =========================================================================

    /**
     * plugin dimensions (e.g., BwsBarometer)
     *
     * Characteristics:
     *   - Hero dial dominates (200-260px)
     *   - Dial at golden ratio height
     *   - Minimal secondary controls
     */
    struct Tier1
    {
        static constexpr int WIDTH = 408;
        static constexpr int HEIGHT = 432;
        static constexpr int DIAL_SIZE = 200;
        static constexpr int MARGIN = 24;
        static constexpr float MIN_SCALE = 0.75f;
        static constexpr float MAX_SCALE = 1.5f;

        /** Get dial center Y position using golden ratio */
        static int getDialCenterY(int totalHeight = HEIGHT)
        {
            return static_cast<int>(static_cast<float>(totalHeight) / PHI);
        }

        /** Get minimum window size */
        static juce::Rectangle<int> getMinSize()
        {
            return {0, 0, static_cast<int>(WIDTH * MIN_SCALE), static_cast<int>(HEIGHT * MIN_SCALE)};
        }

        /** Get maximum window size */
        static juce::Rectangle<int> getMaxSize()
        {
            return {0, 0, static_cast<int>(WIDTH * MAX_SCALE), static_cast<int>(HEIGHT * MAX_SCALE)};
        }
    };

    // =========================================================================
    // Station Plugin (5+ controls, multi-module)
    // =========================================================================

    /**
     * plugin dimensions (e.g., )
     *
     * Characteristics:
     *   - Multiple modules/sections
     *   - Smaller supporting dials (120px)
     *   - Complex processing chains
     */
    struct Tier2
    {
        static constexpr int WIDTH = 600;
        static constexpr int HEIGHT = 400;
        static constexpr int DIAL_SIZE = 120;
        static constexpr int SMALL_DIAL_SIZE = 80;
        static constexpr int MARGIN = 30;
        static constexpr float MIN_SCALE = 0.75f;
        static constexpr float MAX_SCALE = 1.5f;

        /** Get section width using golden ratio */
        static int getSectionWidth(int totalWidth = WIDTH, int numSections = 3)
        {
            const int availableWidth = totalWidth - (MARGIN * 2);
            return availableWidth / numSections;
        }
    };

    // =========================================================================
    // Spacing Utilities
    // =========================================================================

    /**
     * Get spacing at Fibonacci level (golden ratio derived)
     *
     * @param level Fibonacci level (0=8, 1=13, 2=21, 3=34, 4=55...)
     * @param base Base unit (default 8px)
     * @return Spacing value
     *
     * Level 0: 8px (base)
     * Level 1: ~13px (8 × φ)
     * Level 2: ~21px (13 × φ)
     * Level 3: ~34px (21 × φ)
     * Level 4: ~55px (34 × φ)
     */
    static float getSpacing(int level, float base = SPACING_BASE)
    {
        float spacing = base;
        for (int i = 0; i < level; ++i)
            spacing *= PHI;
        return spacing;
    }

    /**
     * Get hero dial center Y using golden ratio
     *
     * @param totalHeight Total plugin height
     * @return Y position for dial center
     */
    static int getHeroDialCenterY(int totalHeight) { return static_cast<int>(static_cast<float>(totalHeight) / PHI); }

    /**
     * Position element at golden ratio point
     *
     * @param bounds Parent bounds
     * @param elementSize Size of element to position
     * @param horizontal If true, use horizontal golden ratio; otherwise vertical
     * @return Positioned rectangle
     */
    static juce::Rectangle<int> positionAtGoldenRatio(const juce::Rectangle<int>& bounds, int elementSize,
                                                      bool horizontal = false)
    {
        if (horizontal)
        {
            const int x =
                static_cast<int>(static_cast<float>(bounds.getWidth()) * PHI_INVERSE) - elementSize / 2 + bounds.getX();
            return {x, bounds.getY(), elementSize, bounds.getHeight()};
        }
        else
        {
            const int y = static_cast<int>(static_cast<float>(bounds.getHeight()) * PHI_INVERSE) - elementSize / 2 +
                          bounds.getY();
            return {bounds.getX(), y, bounds.getWidth(), elementSize};
        }
    }

    /**
     * Split bounds using golden ratio
     *
     * @param bounds Bounds to split
     * @param horizontal If true, split horizontally; otherwise vertically
     * @return Pair of rectangles (larger, smaller)
     */
    static std::pair<juce::Rectangle<int>, juce::Rectangle<int>> splitByGoldenRatio(const juce::Rectangle<int>& bounds,
                                                                                    bool horizontal = false)
    {
        if (horizontal)
        {
            const int splitX = static_cast<int>(static_cast<float>(bounds.getWidth()) * PHI_INVERSE);
            return {bounds.withWidth(splitX), bounds.withTrimmedLeft(splitX)};
        }
        else
        {
            const int splitY = static_cast<int>(static_cast<float>(bounds.getHeight()) * PHI_INVERSE);
            return {bounds.withHeight(splitY), bounds.withTrimmedTop(splitY)};
        }
    }

    // =========================================================================
    // Scale Utilities
    // =========================================================================

    /**
     * Scale value by factor
     *
     * @param base Base value
     * @param scale Scale factor (1.0 = 100%)
     * @return Scaled value
     */
    static int scaled(int base, float scale) { return static_cast<int>(static_cast<float>(base) * scale); }

    /**
     * Scale rectangle by factor
     *
     * @param bounds Base bounds
     * @param scale Scale factor
     * @return Scaled bounds
     */
    static juce::Rectangle<int> scaled(const juce::Rectangle<int>& bounds, float scale)
    {
        return {scaled(bounds.getX(), scale), scaled(bounds.getY(), scale), scaled(bounds.getWidth(), scale),
                scaled(bounds.getHeight(), scale)};
    }

    // =========================================================================
    // Centering Utilities
    // =========================================================================

    /**
     * Center child bounds within parent
     *
     * @param parent Parent bounds
     * @param childWidth Child width
     * @param childHeight Child height
     * @return Centered child bounds
     */
    static juce::Rectangle<int> center(const juce::Rectangle<int>& parent, int childWidth, int childHeight)
    {
        return {parent.getX() + (parent.getWidth() - childWidth) / 2,
                parent.getY() + (parent.getHeight() - childHeight) / 2, childWidth, childHeight};
    }

    /**
     * Center child bounds horizontally within parent
     *
     * @param parent Parent bounds
     * @param childWidth Child width
     * @param y Y position
     * @param height Height
     * @return Horizontally centered bounds
     */
    static juce::Rectangle<int> centerHorizontally(const juce::Rectangle<int>& parent, int childWidth, int y,
                                                   int height)
    {
        return {parent.getX() + (parent.getWidth() - childWidth) / 2, y, childWidth, height};
    }
};

} // namespace bws::weather
