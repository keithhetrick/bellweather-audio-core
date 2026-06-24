// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

/**
 * =============================================================================
 * InteractiveDisplay.h - Interactive Display Base Class
 * =============================================================================
 *
 * Bellweather Studios - Weather Instrument Design System
 *
 * Provides FabFilter-style direct manipulation for graph/curve displays:
 *   - Click to create elements
 *   - Drag to adjust elements
 *   - Multi-select (Ctrl/Cmd+Click)
 *   - Rectangle selection (drag on background)
 *   - Right-click context menu
 *   - Hover preview (show what you'll create)
 *   - Vintage graph paper grid rendering
 *
 * Usage:
 *   class MyGraph : public bws::weather::InteractiveDisplay
 *   {
 *   protected:
 *       void paintVisualization(juce::Graphics& g) override
 *       {
 *           // Draw your curve/graph
 *       }
 *
 *       void createElement(juce::Point<int> position) override
 *       {
 *           // Add a control point
 *       }
 *
 *       int getElementAtPosition(juce::Point<int> pos) const override
 *       {
 *           // Return index of element at position, or -1
 *       }
 *   };
 *
 * =============================================================================
 */

#include <bw_ui/rendering/IRenderer.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <bw_ui/foundation/UiTheme.h>
#include <functional>

namespace bws
{
namespace weather
{

/**
 * @brief Base class for interactive Weather displays.
 *
 * Provides common interaction patterns for graph and curve displays
 * with FabFilter-quality UX while maintaining the Weather aesthetic.
 */
class InteractiveDisplay : public juce::Component
{
public:
    InteractiveDisplay();
    ~InteractiveDisplay() override = default;

    // =========================================================================
    // Interactivity Settings
    // =========================================================================

    /** Enable/disable all interactivity */
    void setInteractive(bool enable)
    {
        interactive_ = enable;
        repaint();
    }
    bool isInteractive() const { return interactive_; }

    /** Enable/disable hover preview (ghost of element to be created) */
    void setShowHoverPreview(bool show)
    {
        showHoverPreview_ = show;
        repaint();
    }
    bool isShowingHoverPreview() const { return showHoverPreview_; }

    /** Enable/disable grid drawing */
    void setShowGrid(bool show)
    {
        showGrid_ = show;
        repaint();
    }
    bool isShowingGrid() const { return showGrid_; }

    /** Set grid style */
    enum class GridStyle
    {
        None,
        Lines,
        Dots,
        GraphPaper
    };
    void setGridStyle(GridStyle style)
    {
        gridStyle_ = style;
        repaint();
    }
    GridStyle getGridStyle() const { return gridStyle_; }

    /** Set grid spacing (in pixels at 1.0 scale) */
    void setGridSpacing(int spacing)
    {
        gridSpacing_ = spacing;
        repaint();
    }
    int getGridSpacing() const { return gridSpacing_; }

    /** Enable/disable snap-to-grid for element placement */
    void setSnapToGrid(bool snap) { snapToGrid_ = snap; }
    bool isSnappingToGrid() const { return snapToGrid_; }

    // =========================================================================
    // Scale Factor (for HiDPI support)
    // =========================================================================

    void setScaleFactor(float scale)
    {
        scaleFactor_ = scale;
        repaint();
    }
    float getScaleFactor() const { return scaleFactor_; }

    /** Apply resolved theme (colors, spacing, typography). */
    void setTheme(const bws::ui::UiThemeResolved& t) { theme_ = &t; }

    // =========================================================================
    // juce::Component Overrides
    // =========================================================================

    void paint(juce::Graphics& g) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    void mouseExit(const juce::MouseEvent& e) override;

protected:
    // =========================================================================
    // Override These in Derived Classes
    // =========================================================================

    /**
     * Override to draw your visualization (curve, graph, etc.).
     * Called after grid is drawn.
     */
    virtual void paintVisualization(juce::Graphics& g) = 0;

    /**
     * Override to draw hover preview (ghost element at cursor position).
     * Called when showHoverPreview_ is true and mouse is over component.
     */
    virtual void paintHoverPreview(juce::Graphics& /*g*/, juce::Point<int> /*position*/) {}

    /**
     * Override to draw selection rectangle during drag-select.
     */
    virtual void paintSelectionRect(juce::Graphics& g, juce::Rectangle<int> rect);

    /**
     * Override to handle element creation on click.
     * - position: Click position in component coordinates
     * - mods: Modifier keys held during click
     */
    virtual void createElement(juce::Point<int> /*position*/, const juce::ModifierKeys& /*mods*/) {}

    /**
     * Override to handle element adjustment during drag.
     * - elementIndex: Index of element being dragged
     * - delta: Movement since last drag event
     * - mods: Modifier keys held during drag
     */
    virtual void adjustElement(int /*elementIndex*/, juce::Point<int> /*delta*/, const juce::ModifierKeys& /*mods*/) {}

    /**
     * Override to handle element double-click (e.g., delete or edit).
     * - elementIndex: Index of element that was double-clicked
     */
    virtual void onElementDoubleClick(int /*elementIndex*/) {}

    /**
     * Override to get element at a position.
     * @return Index of element at position, or -1 if none
     */
    virtual int getElementAtPosition(juce::Point<int> /*position*/) const { return -1; }

    /**
     * Override to get elements within a rectangle (for multi-select).
     * @return Indices of elements within the rectangle
     */
    virtual juce::Array<int> getElementsInRect(juce::Rectangle<int> /*rect*/) const { return {}; }

    /**
     * Override to show context menu for element or background.
     * - position: Click position
     * - elementIndex: Index of element clicked, or -1 for background
     */
    virtual void showContextMenu(juce::Point<int> /*position*/, int /*elementIndex*/) {}

    /**
     * Override to handle mouse wheel on element.
     * - elementIndex: Index of element under cursor, or -1
     * - wheel: Wheel details
     * - mods: Modifier keys
     * @return true if handled
     */
    virtual bool onElementMouseWheel(int /*elementIndex*/, const juce::MouseWheelDetails& /*wheel*/,
                                     const juce::ModifierKeys& /*mods*/)
    {
        return false;
    }

    // =========================================================================
    // Selection Management
    // =========================================================================

    /** Select a single element (clear others unless addToSelection) */
    void selectElement(int index, bool addToSelection = false);

    /** Toggle element selection */
    void toggleElementSelection(int index);

    /** Select multiple elements */
    void selectElements(const juce::Array<int>& indices, bool addToSelection = false);

    /** Clear all selections */
    void clearSelection();

    /** Check if element is selected */
    bool isElementSelected(int index) const;

    /** Get all selected element indices */
    const juce::Array<int>& getSelectedElements() const { return selectedElements_; }

    /** Get number of selected elements */
    int getNumSelectedElements() const { return selectedElements_.size(); }

    // =========================================================================
    // Grid Drawing Helpers
    // =========================================================================

    /** Draw vintage graph paper style grid (IRenderer) */
    void drawVintageGrid(bws::ui::rendering::IRenderer& r);

    /** Draw simple line grid (IRenderer) */
    void drawLineGrid(bws::ui::rendering::IRenderer& r);

    /** Draw dot grid (IRenderer) */
    void drawDotGrid(bws::ui::rendering::IRenderer& r);

    /** Snap a point to the grid */
    juce::Point<int> snapToGridPoint(juce::Point<int> point) const;

    // =========================================================================
    // Coordinate Conversion Helpers
    // =========================================================================

    /** Scale a dimension by the current scale factor */
    int scaled(int value) const { return juce::roundToInt(static_cast<float>(value) * scaleFactor_); }
    float scaledF(float value) const { return value * scaleFactor_; }

    /** Resolved theme for subclass visualization (null until setTheme). */
    const bws::ui::UiThemeResolved* getTheme() const noexcept { return theme_; }

    // =========================================================================
    // Callbacks
    // =========================================================================

    /** Callback when selection changes */
    std::function<void()> onSelectionChanged;

    /** Callback when an element is created */
    std::function<void(int elementIndex)> onElementCreated;

    /** Callback when an element is moved */
    std::function<void(int elementIndex, juce::Point<int> newPosition)> onElementMoved;

    /** Callback when an element is deleted */
    std::function<void(int elementIndex)> onElementDeleted;

private:
    // =========================================================================
    // State
    // =========================================================================

    bool interactive_ = true;
    bool showHoverPreview_ = true;
    bool showGrid_ = true;
    bool snapToGrid_ = false;
    GridStyle gridStyle_ = GridStyle::GraphPaper;
    int gridSpacing_ = 20;
    float scaleFactor_ = 1.0f;

    const bws::ui::UiThemeResolved* theme_ = nullptr;

    // Selection state
    juce::Array<int> selectedElements_;

    // Interaction state
    juce::Point<int> hoverPosition_;
    bool isHovering_ = false;

    // Drag state
    bool isDragging_ = false;
    bool isDragSelecting_ = false;
    int draggedElement_ = -1;
    juce::Point<int> dragStartPosition_;
    juce::Point<int> lastDragPosition_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InteractiveDisplay)
};

} // namespace weather
} // namespace bws
