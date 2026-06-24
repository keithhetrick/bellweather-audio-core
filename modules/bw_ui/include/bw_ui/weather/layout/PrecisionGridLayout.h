// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <bw_ui/foundation/GoldenRatioConstants.h>
#include <vector>
#include <functional>

namespace bws::weather
{

/**
 * @class PrecisionGridLayout
 * @brief Multi-row grid layout manager for precision controls
 *
 * FEATURES:
 * - Multiple rows with different column counts per row
 * - Circular safe zone support (for iris aperture, spotlight effects)
 * - Golden ratio spacing (φ-based)
 * - Auto-centering within bounds
 * - Responsive sizing with configurable constraints
 * - Toggle row anchoring to knob columns
 *
 * USAGE:
 * @code
 * auto grid = std::make_unique<PrecisionGridLayout>();
 * grid->addKnobRow({imageKnob, roundKnob, airKnob, anchorKnob});  // 4 controls
 * grid->addKnobRow({threshKnob, ratioKnob, attackKnob, ...});     // 7 controls
 * grid->addToggleRow({hpfToggle, ratioToggle, autoToggle});       // 3 toggles
 * grid->layoutInBounds(content->getLocalBounds(), 0.60f);         // 60% safe zone
 * @endcode
 */
class PrecisionGridLayout
{
public:
    PrecisionGridLayout() = default;
    ~PrecisionGridLayout() = default;

    //==========================================================================
    // ROW CONFIGURATION
    //==========================================================================

    /**
     * @brief Control info for layout (knob or slider)
     */
    struct ControlInfo
    {
        juce::Component* control; ///< The knob/slider component
        juce::Component* label;   ///< Associated label (below control)
    };

    /**
     * @brief Toggle info for layout
     */
    struct ToggleInfo
    {
        juce::Component* toggle; ///< The toggle button
        juce::Component* label;  ///< Associated label (below toggle)
    };

    /**
     * @brief Add a row of knobs/sliders
     * @param controls Vector of controls with labels
     */
    void addKnobRow(const std::vector<ControlInfo>& controls);

    /**
     * @brief Add a row of toggle buttons
     * @param toggles Vector of toggles with labels
     * @param anchorToFirstRow If true, toggles align under first knob row
     */
    void addToggleRow(const std::vector<ToggleInfo>& toggles, bool anchorToFirstRow = true);

    /**
     * @brief Clear all rows
     */
    void clearRows();

    //==========================================================================
    // LAYOUT EXECUTION
    //==========================================================================

    /**
     * @brief Layout all controls within bounds
     * @param bounds Available area for layout
     * @param safeZoneFactor Factor for circular constraint (0.5-1.0). Default 1.0 = no constraint
     *
     * Example: 0.60 = 60% safe zone for iris aperture blade clearance
     */
    void layoutInBounds(juce::Rectangle<int> bounds, float safeZoneFactor = 1.0f);

    //==========================================================================
    // SIZE CONSTRAINTS
    //==========================================================================

    /**
     * @brief Set knob size range
     * @param minSize Minimum knob size (default: 28px)
     * @param maxSize Maximum knob size (default: 55px = 89/φ)
     */
    void setKnobSizeRange(int minSize, int maxSize);

    /**
     * @brief Set toggle size as factor of knob size
     * @param factor Multiplier (default: 0.35)
     */
    void setToggleSizeFactor(float factor);

    /**
     * @brief Set label height (default: kPrecisionLabelHeight ~10px)
     * @param height Label height in pixels
     */
    void setLabelHeight(int height);

    /**
     * @brief Set label gap (default: kPrecisionLabelGap ~3px)
     * @param gap Gap between control and label in pixels
     */
    void setLabelGap(int gap);

    /**
     * @brief Get the calculated grid dimensions after layout
     * @return Rectangle containing the grid (for centering validation)
     */
    juce::Rectangle<int> getGridBounds() const { return lastGridBounds_; }

    /**
     * @brief Get the calculated knob size from last layout
     * @return Knob size in pixels
     */
    int getLastKnobSize() const { return lastKnobSize_; }

private:
    //==========================================================================
    // INTERNAL STRUCTURES
    //==========================================================================

    struct KnobRow
    {
        std::vector<ControlInfo> controls;
    };

    struct ToggleRow
    {
        std::vector<ToggleInfo> toggles;
        bool anchorToFirstRow;
    };

    //==========================================================================
    // INTERNAL METHODS
    //==========================================================================

    /**
     * @brief Calculate optimal knob size from width constraint
     */
    int calculateKnobSizeFromWidth(int availableWidth, int maxColumns, float spacingRatio) const;

    /**
     * @brief Calculate optimal knob size from height constraint
     */
    int calculateKnobSizeFromHeight(int availableHeight, int knobRows, bool hasToggleRow, float spacingRatio) const;

    /**
     * @brief Layout a single knob row
     * @param row The row to layout
     * @param gridStartX Starting X position of the grid
     * @param gridWidth Total width of the grid
     * @param startY Y position for this row
     * @param knobSize Size of each knob
     * @param hSpacing Horizontal spacing between knobs
     */
    void layoutKnobRow(const KnobRow& row, int gridStartX, int gridWidth, int startY, int knobSize, int hSpacing);

    /**
     * @brief Layout toggle row with optional anchoring
     * @param row The toggle row to layout
     * @param firstKnobRow The first knob row (for anchoring calculations)
     * @param gridStartX Starting X position of the grid
     * @param gridWidth Total width of the grid
     * @param startY Y position for this row
     * @param knobSize Size of knobs (for proportional toggle sizing)
     * @param hSpacing Horizontal spacing
     */
    void layoutToggleRow(const ToggleRow& row, const KnobRow* firstKnobRow, int gridStartX, int gridWidth, int startY,
                         int knobSize, int hSpacing);

    //==========================================================================
    // MEMBER VARIABLES
    //==========================================================================

    std::vector<KnobRow> knobRows_;
    std::vector<ToggleRow> toggleRows_;

    // Size constraints
    int minKnobSize_ = 28;
    int maxKnobSize_ = 75;           // Increased for better fill at default size
    float toggleSizeFactor_ = 0.45f; // Toggle height as factor of knob size (was 0.35, increased for visual balance)
    int labelHeight_ = bws::design::kPrecisionLabelHeight; // ~10px
    int labelGap_ = bws::design::kPrecisionLabelGap;       // ~3px

    // Last layout result (for validation/debugging)
    juce::Rectangle<int> lastGridBounds_;
    int lastKnobSize_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrecisionGridLayout)
};

} // namespace bws::weather
