// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include <bw_ui/weather/layout/PrecisionGridLayout.h>
#include <algorithm>
#include <cmath>

namespace bws::weather
{

using namespace bws::design;

//==============================================================================
// ROW CONFIGURATION
//==============================================================================

void PrecisionGridLayout::addKnobRow(const std::vector<ControlInfo>& controls)
{
    KnobRow row;
    row.controls = controls;
    knobRows_.push_back(std::move(row));
}

void PrecisionGridLayout::addToggleRow(const std::vector<ToggleInfo>& toggles, bool anchorToFirstRow)
{
    ToggleRow row;
    row.toggles = toggles;
    row.anchorToFirstRow = anchorToFirstRow;
    toggleRows_.push_back(std::move(row));
}

void PrecisionGridLayout::clearRows()
{
    knobRows_.clear();
    toggleRows_.clear();
    lastGridBounds_ = {};
    lastKnobSize_ = 0;
}

//==============================================================================
// SIZE CONSTRAINTS
//==============================================================================

void PrecisionGridLayout::setKnobSizeRange(int minSize, int maxSize)
{
    minKnobSize_ = minSize;
    maxKnobSize_ = maxSize;
}

void PrecisionGridLayout::setToggleSizeFactor(float factor)
{
    toggleSizeFactor_ = factor;
}

void PrecisionGridLayout::setLabelHeight(int height)
{
    labelHeight_ = height;
}

void PrecisionGridLayout::setLabelGap(int gap)
{
    labelGap_ = gap;
}

//==============================================================================
// LAYOUT EXECUTION
//==============================================================================

void PrecisionGridLayout::layoutInBounds(juce::Rectangle<int> bounds, float safeZoneFactor)
{
    if (bounds.isEmpty() || knobRows_.empty())
        return;

    const int availableWidth = bounds.getWidth();
    const int availableHeight = bounds.getHeight();

    // Apply safe zone factor (for circular constraints like iris aperture)
    const int safeWidth = static_cast<int>(availableWidth * safeZoneFactor);
    const int safeHeight = static_cast<int>(availableHeight * safeZoneFactor);

    // Find max columns across all knob rows (determines grid width)
    int maxCols = 0;
    for (const auto& row : knobRows_)
    {
        maxCols = std::max(maxCols, static_cast<int>(row.controls.size()));
    }

    if (maxCols == 0)
        return;

    const int knobRows = static_cast<int>(knobRows_.size());
    const bool hasToggleRow = !toggleRows_.empty();

    // Unified spacing constants (golden ratio derived)
    const float spacingRatio = kPrecisionSpacingRatio; // ~0.155 (φ-based)

    // Calculate knob size from width constraint
    int knobSizeFromWidth = calculateKnobSizeFromWidth(safeWidth, maxCols, spacingRatio);

    // Calculate knob size from height constraint
    int knobSizeFromHeight = calculateKnobSizeFromHeight(safeHeight, knobRows, hasToggleRow, spacingRatio);

    // Use smaller, clamp to min/max range
    int knobSize = std::min(knobSizeFromWidth, knobSizeFromHeight);
    knobSize = std::max(minKnobSize_, std::min(maxKnobSize_, knobSize));
    lastKnobSize_ = knobSize;

    // Calculate spacing
    int hSpacing = static_cast<int>(knobSize * spacingRatio);
    int vSpacing = static_cast<int>(knobSize * spacingRatio);
    int cellHeight = knobSize + labelHeight_ + labelGap_;

    // Toggle dimensions
    int toggleHeight = std::max(14, static_cast<int>(knobSize * toggleSizeFactor_));
    int toggleCellHeight = toggleHeight + labelGap_ + labelHeight_;

    // Grid dimensions
    int gridWidth = maxCols * knobSize + (maxCols - 1) * hSpacing;
    int gridHeight = knobRows * cellHeight + (knobRows - 1) * vSpacing;

    // Add toggle row height if present
    if (hasToggleRow)
    {
        gridHeight += vSpacing + toggleCellHeight;
    }

    // Center grid in available bounds
    int gridStartX = bounds.getX() + (availableWidth - gridWidth) / 2;
    int startY = bounds.getY() + (availableHeight - gridHeight) / 2;

    // Store grid bounds for validation
    lastGridBounds_ = juce::Rectangle<int>(gridStartX, startY, gridWidth, gridHeight);

    DBG("=== PrecisionGridLayout ===");
    DBG("Bounds: " << bounds.toString() << ", Safe zone: " << safeZoneFactor);
    DBG("Knob size: " << knobSize << "px (w:" << knobSizeFromWidth << ", h:" << knobSizeFromHeight << ")");
    DBG("Grid: " << gridWidth << " x " << gridHeight << " at (" << gridStartX << ", " << startY << ")");

    // Layout each knob row
    int currentY = startY;
    for (size_t rowIdx = 0; rowIdx < knobRows_.size(); ++rowIdx)
    {
        layoutKnobRow(knobRows_[rowIdx], gridStartX, gridWidth, currentY, knobSize, hSpacing);
        currentY += cellHeight + vSpacing;
    }

    // Remove the extra vSpacing added in the loop (last row doesn't need it before toggles)
    currentY -= vSpacing;

    // Layout toggle rows
    if (hasToggleRow)
    {
        // Use the same vSpacing as between knob rows for visual consistency
        // This ensures the gap from the last knob row's labels to the toggle row
        // matches the gap between knob rows
        currentY += vSpacing;

        for (auto& toggleRow : toggleRows_)
        {
            const KnobRow* firstRow = toggleRow.anchorToFirstRow && !knobRows_.empty() ? &knobRows_[0] : nullptr;

            layoutToggleRow(toggleRow, firstRow, gridStartX, gridWidth, currentY, knobSize, hSpacing);
            currentY += toggleCellHeight + vSpacing;
        }
    }

    // Centering validation
    int contentTop = startY;
    int contentBottom = currentY - vSpacing; // Remove trailing spacing
    int actualContentHeight = contentBottom - contentTop;
    int expectedCenterY = bounds.getY() + availableHeight / 2;
    int actualCenterY = contentTop + actualContentHeight / 2;

    DBG("=== CENTERING VALIDATION ===");
    DBG("Content: top=" << contentTop << " bottom=" << contentBottom << " height=" << actualContentHeight);
    DBG("Expected center Y: " << expectedCenterY << ", Actual center Y: " << actualCenterY);
    DBG("Centering offset: " << (actualCenterY - expectedCenterY) << "px");

    if (std::abs(actualCenterY - expectedCenterY) > 5)
    {
        DBG("WARNING: Content not centered - offset exceeds 5px");
    }
    else
    {
        DBG("OK: Content properly centered");
    }
}

//==============================================================================
// INTERNAL METHODS
//==============================================================================

int PrecisionGridLayout::calculateKnobSizeFromWidth(int availableWidth, int maxColumns, float spacingRatio) const
{
    // widthFactor = cols + (cols - 1) * spacingRatio
    float widthFactor = maxColumns + (maxColumns - 1) * spacingRatio;
    return static_cast<int>(availableWidth / widthFactor);
}

int PrecisionGridLayout::calculateKnobSizeFromHeight(int availableHeight, int knobRows, bool hasToggleRow,
                                                     float spacingRatio) const
{
    // Height calculation:
    // knobRows * (knobSize + labelHeight + labelGap) + (knobRows - 1) * (knobSize * spacingRatio)
    // + [if toggle row: spacingRatio * knobSize + (toggleSizeFactor * knobSize + labelGap + labelHeight)]
    //
    // Solving for knobSize:
    // heightConstant = knobRows * (labelHeight + labelGap) + [toggleRow: labelHeight + labelGap]
    // heightFactor = knobRows + (knobRows - 1) * spacingRatio + [toggleRow: toggleSizeFactor + spacingRatio]

    float heightConstant = knobRows * (labelHeight_ + labelGap_);
    float heightFactor = knobRows + (knobRows - 1) * spacingRatio;

    if (hasToggleRow)
    {
        heightConstant += labelHeight_ + labelGap_;
        heightFactor += toggleSizeFactor_ + spacingRatio;
    }

    return static_cast<int>((availableHeight - heightConstant) / heightFactor);
}

void PrecisionGridLayout::layoutKnobRow(const KnobRow& row, int gridStartX, int gridWidth, int startY, int knobSize,
                                        int hSpacing)
{
    const int numControls = static_cast<int>(row.controls.size());
    if (numControls == 0)
        return;

    // Calculate row width and center it within the grid
    int rowWidth = numControls * knobSize + (numControls - 1) * hSpacing;
    int rowStartX = gridStartX + (gridWidth - rowWidth) / 2;

    for (int col = 0; col < numControls; ++col)
    {
        const auto& ctrl = row.controls[col];
        if (!ctrl.control)
            continue;

        int x = rowStartX + col * (knobSize + hSpacing);
        int y = startY;

        ctrl.control->setBounds(x, y, knobSize, knobSize);

        if (ctrl.label)
        {
            ctrl.label->setBounds(x, y + knobSize + labelGap_, knobSize, labelHeight_);
        }
    }
}

void PrecisionGridLayout::layoutToggleRow(const ToggleRow& row, const KnobRow* firstKnobRow, int gridStartX,
                                          int gridWidth, int startY, int knobSize, int hSpacing)
{
    const int numToggles = static_cast<int>(row.toggles.size());
    if (numToggles == 0)
        return;

    // Toggle dimensions - make toggles proportional to their height for visual balance
    // Width should be similar to height for a more balanced toggle appearance
    int toggleHeight = std::max(14, static_cast<int>(knobSize * toggleSizeFactor_));
    // Toggle width matches the knob size for consistent column alignment
    // This ensures toggle labels align with knob labels above
    int toggleWidth = knobSize;

    // Calculate toggle positions
    std::vector<int> toggleCentersX;

    if (row.anchorToFirstRow && firstKnobRow && firstKnobRow->controls.size() >= 2)
    {
        // Anchor toggles to first knob row
        // Visual anchoring pattern:
        //   |--K1-----K2-----K3-----K4--|  (N knobs)
        //   |--T1-----------T2----------T3--|  (M toggles: outer align to K1/KN, middle centered)
        //
        // Toggle 1: centered under knob 1
        // Toggle M: centered under knob N
        // Middle toggles: evenly distributed between

        const int numKnobs = static_cast<int>(firstKnobRow->controls.size());
        int firstRowWidth = numKnobs * knobSize + (numKnobs - 1) * hSpacing;
        int firstRowStartX = gridStartX + (gridWidth - firstRowWidth) / 2;

        // Calculate knob center positions
        int knob1CenterX = firstRowStartX + knobSize / 2;
        int knobNCenterX = firstRowStartX + (numKnobs - 1) * (knobSize + hSpacing) + knobSize / 2;

        if (numToggles == 1)
        {
            // Single toggle: center between first and last knob
            toggleCentersX.push_back((knob1CenterX + knobNCenterX) / 2);
        }
        else if (numToggles == 2)
        {
            // Two toggles: align to first and last knob
            toggleCentersX.push_back(knob1CenterX);
            toggleCentersX.push_back(knobNCenterX);
        }
        else
        {
            // Three or more toggles: first under K1, last under KN, middle toggles distributed
            toggleCentersX.push_back(knob1CenterX);

            // Calculate center point for middle toggles
            int rowCenterX = (knob1CenterX + knobNCenterX) / 2;

            // For 3 toggles: outer align to K1/KN, middle at center
            // For more: distribute evenly
            if (numToggles == 3)
            {
                toggleCentersX.push_back(rowCenterX);
            }
            else
            {
                // Distribute middle toggles evenly
                int middleCount = numToggles - 2;
                float step = static_cast<float>(knobNCenterX - knob1CenterX) / (numToggles - 1);
                for (int i = 1; i <= middleCount; ++i)
                {
                    toggleCentersX.push_back(knob1CenterX + static_cast<int>(i * step));
                }
            }

            toggleCentersX.push_back(knobNCenterX);
        }
    }
    else
    {
        // No anchoring - center toggles independently
        int toggleRowWidth = numToggles * toggleWidth + (numToggles - 1) * hSpacing;
        int toggleRowStartX = gridStartX + (gridWidth - toggleRowWidth) / 2;

        for (int i = 0; i < numToggles; ++i)
        {
            int x = toggleRowStartX + i * (toggleWidth + hSpacing);
            toggleCentersX.push_back(x + toggleWidth / 2);
        }
    }

    // Layout toggles at calculated positions
    for (int i = 0; i < numToggles && i < static_cast<int>(toggleCentersX.size()); ++i)
    {
        const auto& tog = row.toggles[i];
        if (!tog.toggle)
            continue;

        int centerX = toggleCentersX[i];
        int x = centerX - toggleWidth / 2;

        tog.toggle->setBounds(x, startY, toggleWidth, toggleHeight);

        if (tog.label)
        {
            // Label uses same dimensions as knob labels for consistency
            tog.label->setBounds(centerX - knobSize / 2, startY + toggleHeight + labelGap_, knobSize, labelHeight_);
        }
    }
}

} // namespace bws::weather
