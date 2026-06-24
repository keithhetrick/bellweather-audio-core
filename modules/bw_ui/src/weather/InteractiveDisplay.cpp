// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

/**
 * =============================================================================
 * InteractiveDisplay.cpp - Implementation
 * =============================================================================
 */

#include "bw_ui/weather/displays/InteractiveDisplay.h"
#include "bw_ui/generated/BwTokens.h"
#include <bw_ui/rendering/JuceRenderer.h>
#include "bw_ui/foundation/UiTheme.h"

namespace bws
{
namespace weather
{

// =============================================================================
// InteractiveDisplay Implementation
// =============================================================================

InteractiveDisplay::InteractiveDisplay()
{
    setWantsKeyboardFocus(true);
}

// =============================================================================
// Paint
// =============================================================================

void InteractiveDisplay::paint(juce::Graphics& g)
{
    static const auto kDefault = bws::ui::resolveTheme(bws::ui::defaultUiThemeBase(), {});
    const auto& t = theme_ ? *theme_ : kDefault;

    bws::ui::rendering::JuceRenderer renderer(g);

    // Fill background
    auto bounds = getLocalBounds().toFloat();
    renderer.setColour(t.weatherColors.bgInput.getARGB());
    renderer.fillRect(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight());

    // Draw grid if enabled (IRenderer)
    if (showGrid_)
    {
        switch (gridStyle_)
        {
        case GridStyle::Lines:
            drawLineGrid(renderer);
            break;
        case GridStyle::Dots:
            drawDotGrid(renderer);
            break;
        case GridStyle::GraphPaper:
            drawVintageGrid(renderer);
            break;
        case GridStyle::None:
        default:
            break;
        }
    }

    // Virtual dispatch stays on raw g (subclasses override these)
    paintVisualization(g);

    if (interactive_ && showHoverPreview_ && isHovering_ && draggedElement_ < 0)
    {
        paintHoverPreview(g, hoverPosition_);
    }

    if (isDragSelecting_)
    {
        auto selectionRect = juce::Rectangle<int>(dragStartPosition_, lastDragPosition_);
        paintSelectionRect(g, selectionRect);
    }
}

// =============================================================================
// Mouse Handling
// =============================================================================

void InteractiveDisplay::mouseMove(const juce::MouseEvent& e)
{
    if (!interactive_)
        return;

    hoverPosition_ = e.getPosition();
    isHovering_ = true;

    if (showHoverPreview_)
        repaint();
}

void InteractiveDisplay::mouseDown(const juce::MouseEvent& e)
{
    if (!interactive_)
        return;

    dragStartPosition_ = e.getPosition();
    lastDragPosition_ = e.getPosition();

    // Check if clicking on an element
    int elementIndex = getElementAtPosition(e.getPosition());

    if (e.mods.isRightButtonDown())
    {
        // Right-click: Show context menu
        showContextMenu(e.getPosition(), elementIndex);
        return;
    }

    if (elementIndex >= 0)
    {
        // Clicked on an element
        draggedElement_ = elementIndex;
        isDragging_ = true;

        // Handle selection based on modifiers
        if (e.mods.isCommandDown() || e.mods.isCtrlDown())
        {
            // Cmd/Ctrl+click: Toggle selection
            toggleElementSelection(elementIndex);
        }
        else if (e.mods.isShiftDown())
        {
            // Shift+click: Add to selection
            selectElement(elementIndex, true);
        }
        else
        {
            // Normal click: Select if not already selected
            if (!isElementSelected(elementIndex))
            {
                selectElement(elementIndex, false);
            }
        }
    }
    else
    {
        // Clicked on background
        if (!e.mods.isCommandDown() && !e.mods.isCtrlDown() && !e.mods.isShiftDown())
        {
            // Clear selection and start rectangle selection
            clearSelection();
            isDragSelecting_ = true;
        }
        else if (e.mods.isAltDown())
        {
            // Alt+click: Create element at position
            auto position = snapToGrid_ ? snapToGridPoint(e.getPosition()) : e.getPosition();
            createElement(position, e.mods);
        }
    }

    repaint();
}

void InteractiveDisplay::mouseDrag(const juce::MouseEvent& e)
{
    if (!interactive_)
        return;

    auto delta = e.getPosition() - lastDragPosition_;
    lastDragPosition_ = e.getPosition();

    if (isDragSelecting_)
    {
        // Update selection rectangle
        repaint();
    }
    else if (isDragging_ && draggedElement_ >= 0)
    {
        // Drag element(s)
        auto adjustedDelta = delta;

        if (snapToGrid_)
        {
            // Snap movement to grid
            auto currentPos = e.getPosition();
            auto snappedPos = snapToGridPoint(currentPos);
            auto snappedStart = snapToGridPoint(dragStartPosition_);
            adjustedDelta = snappedPos - snappedStart;
        }

        // Move all selected elements
        for (auto selectedIndex : selectedElements_)
        {
            adjustElement(selectedIndex, adjustedDelta, e.mods);
        }

        // If dragged element isn't selected, move it too
        if (!isElementSelected(draggedElement_))
        {
            adjustElement(draggedElement_, adjustedDelta, e.mods);
        }

        repaint();
    }
}

void InteractiveDisplay::mouseUp(const juce::MouseEvent& e)
{
    if (!interactive_)
        return;

    if (isDragSelecting_)
    {
        // Complete rectangle selection
        auto selectionRect = juce::Rectangle<int>(dragStartPosition_, lastDragPosition_);

        if (selectionRect.getWidth() > 5 || selectionRect.getHeight() > 5)
        {
            auto elementsInRect = getElementsInRect(selectionRect);

            if (e.mods.isCommandDown() || e.mods.isCtrlDown())
            {
                // Toggle selection of elements in rect
                for (auto index : elementsInRect)
                    toggleElementSelection(index);
            }
            else
            {
                selectElements(elementsInRect, e.mods.isShiftDown());
            }
        }
        else if (!e.mods.isShiftDown())
        {
            // Single click on background: create element
            auto position = snapToGrid_ ? snapToGridPoint(e.getPosition()) : e.getPosition();
            createElement(position, e.mods);
        }

        isDragSelecting_ = false;
    }

    isDragging_ = false;
    draggedElement_ = -1;

    repaint();
}

void InteractiveDisplay::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (!interactive_)
        return;

    int elementIndex = getElementAtPosition(e.getPosition());

    if (elementIndex >= 0)
    {
        onElementDoubleClick(elementIndex);
    }
    else
    {
        // Double-click on background: create element
        auto position = snapToGrid_ ? snapToGridPoint(e.getPosition()) : e.getPosition();
        createElement(position, e.mods);
    }

    repaint();
}

void InteractiveDisplay::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (!interactive_)
        return;

    int elementIndex = getElementAtPosition(e.getPosition());

    if (onElementMouseWheel(elementIndex, wheel, e.mods))
    {
        repaint();
    }
}

void InteractiveDisplay::mouseExit(const juce::MouseEvent&)
{
    isHovering_ = false;

    if (showHoverPreview_)
        repaint();
}

// =============================================================================
// Selection Management
// =============================================================================

void InteractiveDisplay::selectElement(int index, bool addToSelection)
{
    if (!addToSelection)
        selectedElements_.clear();

    if (index >= 0 && !selectedElements_.contains(index))
        selectedElements_.add(index);

    if (onSelectionChanged)
        onSelectionChanged();

    repaint();
}

void InteractiveDisplay::toggleElementSelection(int index)
{
    if (index < 0)
        return;

    int existingIndex = selectedElements_.indexOf(index);

    if (existingIndex >= 0)
        selectedElements_.remove(existingIndex);
    else
        selectedElements_.add(index);

    if (onSelectionChanged)
        onSelectionChanged();

    repaint();
}

void InteractiveDisplay::selectElements(const juce::Array<int>& indices, bool addToSelection)
{
    if (!addToSelection)
        selectedElements_.clear();

    for (auto index : indices)
    {
        if (index >= 0 && !selectedElements_.contains(index))
            selectedElements_.add(index);
    }

    if (onSelectionChanged)
        onSelectionChanged();

    repaint();
}

void InteractiveDisplay::clearSelection()
{
    if (selectedElements_.isEmpty())
        return;

    selectedElements_.clear();

    if (onSelectionChanged)
        onSelectionChanged();

    repaint();
}

bool InteractiveDisplay::isElementSelected(int index) const
{
    return selectedElements_.contains(index);
}

// =============================================================================
// Grid Drawing
// =============================================================================

void InteractiveDisplay::drawVintageGrid(bws::ui::rendering::IRenderer& r)
{
    static const auto kDefault = bws::ui::resolveTheme(bws::ui::defaultUiThemeBase(), {});
    const auto& t = theme_ ? *theme_ : kDefault;

    const auto bounds = getLocalBounds();
    const int spacing = scaled(gridSpacing_);
    const float w = static_cast<float>(bounds.getWidth());
    const float h = static_cast<float>(bounds.getHeight());

    // Background color (slightly different from main bg for paper texture feel)
    r.setColour(t.weatherColors.bgInput.brighter(0.02f).getARGB());
    r.fillRect(0.0f, 0.0f, w, h);

    // Major grid lines (every 5 minor lines)
    r.setColour(
        t.weatherColors.borderLight.withAlpha(bws::tokens::shared::opacity::display::BORDER_STANDARD).getARGB());
    const int majorSpacing = spacing * 5;

    for (int x = majorSpacing; x < bounds.getWidth(); x += majorSpacing)
        r.drawLine(static_cast<float>(x), 0.0f, static_cast<float>(x), h, 1.0f);

    for (int y = majorSpacing; y < bounds.getHeight(); y += majorSpacing)
        r.drawLine(0.0f, static_cast<float>(y), w, static_cast<float>(y), 1.0f);

    // Minor grid lines
    r.setColour(t.weatherColors.borderLight.withAlpha(bws::tokens::shared::opacity::display::BORDER_FADED).getARGB());

    for (int x = spacing; x < bounds.getWidth(); x += spacing)
    {
        if (x % majorSpacing != 0)
            r.drawLine(static_cast<float>(x), 0.0f, static_cast<float>(x), h, 1.0f);
    }

    for (int y = spacing; y < bounds.getHeight(); y += spacing)
    {
        if (y % majorSpacing != 0)
            r.drawLine(0.0f, static_cast<float>(y), w, static_cast<float>(y), 1.0f);
    }

    // Center crosshair (if applicable)
    if (bounds.getWidth() > spacing * 4 && bounds.getHeight() > spacing * 4)
    {
        const float centerX = static_cast<float>(bounds.getWidth() / 2);
        const float centerY = static_cast<float>(bounds.getHeight() / 2);

        r.setColour(t.weatherColors.accent.withAlpha(bws::tokens::shared::opacity::display::ACCENT_WASH).getARGB());
        r.drawLine(centerX, 0.0f, centerX, h, 1.0f);
        r.drawLine(0.0f, centerY, w, centerY, 1.0f);
    }
}

void InteractiveDisplay::drawLineGrid(bws::ui::rendering::IRenderer& r)
{
    static const auto kDefault = bws::ui::resolveTheme(bws::ui::defaultUiThemeBase(), {});
    const auto& t = theme_ ? *theme_ : kDefault;

    const auto bounds = getLocalBounds();
    const int spacing = scaled(gridSpacing_);
    const float w = static_cast<float>(bounds.getWidth());
    const float h = static_cast<float>(bounds.getHeight());

    r.setColour(t.weatherColors.borderLight.withAlpha(bws::tokens::shared::opacity::display::BORDER_SUBTLE).getARGB());

    for (int x = spacing; x < bounds.getWidth(); x += spacing)
        r.drawLine(static_cast<float>(x), 0.0f, static_cast<float>(x), h, 1.0f);

    for (int y = spacing; y < bounds.getHeight(); y += spacing)
        r.drawLine(0.0f, static_cast<float>(y), w, static_cast<float>(y), 1.0f);
}

void InteractiveDisplay::drawDotGrid(bws::ui::rendering::IRenderer& r)
{
    static const auto kDefault = bws::ui::resolveTheme(bws::ui::defaultUiThemeBase(), {});
    const auto& t = theme_ ? *theme_ : kDefault;

    const auto bounds = getLocalBounds();
    const int spacing = scaled(gridSpacing_);
    const float dotSize = scaledF(2.0f);

    r.setColour(
        t.weatherColors.borderLight.withAlpha(bws::tokens::shared::opacity::display::BORDER_STANDARD).getARGB());

    for (int x = spacing; x < bounds.getWidth(); x += spacing)
    {
        for (int y = spacing; y < bounds.getHeight(); y += spacing)
        {
            r.fillEllipse(x - dotSize * 0.5f, y - dotSize * 0.5f, dotSize, dotSize);
        }
    }
}

// =============================================================================
// Selection Rectangle
// =============================================================================

void InteractiveDisplay::paintSelectionRect(juce::Graphics& g, juce::Rectangle<int> rect)
{
    static const auto kDefault = bws::ui::resolveTheme(bws::ui::defaultUiThemeBase(), {});
    const auto& t = theme_ ? *theme_ : kDefault;

    // Normalize rectangle (handle negative dimensions from right-to-left drag)
    auto normalizedRect = rect.withWidth(std::abs(rect.getWidth())).withHeight(std::abs(rect.getHeight()));

    if (rect.getWidth() < 0)
        normalizedRect.setX(rect.getX() + rect.getWidth());
    if (rect.getHeight() < 0)
        normalizedRect.setY(rect.getY() + rect.getHeight());

    // Fill with semi-transparent brass
    g.setColour(t.weatherColors.accent.withAlpha(bws::tokens::shared::opacity::display::ACCENT_DIM));
    g.fillRect(normalizedRect);

    // Border
    g.setColour(t.weatherColors.accent.withAlpha(bws::tokens::shared::opacity::display::ACCENT_BRIGHT));
    g.drawRect(normalizedRect, 1);
}

// =============================================================================
// Grid Snapping
// =============================================================================

juce::Point<int> InteractiveDisplay::snapToGridPoint(juce::Point<int> point) const
{
    const int spacing = scaled(gridSpacing_);

    int snappedX = juce::roundToInt(static_cast<float>(point.x) / spacing) * spacing;
    int snappedY = juce::roundToInt(static_cast<float>(point.y) / spacing) * spacing;

    return {snappedX, snappedY};
}

} // namespace weather
} // namespace bws
