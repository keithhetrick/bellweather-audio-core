// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

/**
 * =============================================================================
 * UiResizeCorner.h - Custom resize corner component for Bellweather Studios
 * =============================================================================
 *
 * Concentric arcs design (radar/weather station aesthetic):
 *   - Three arcs emanating from bottom-right corner
 *   - Brass gold color matching theme (`#D4AF37`)
 *   - Two visual states: idle (50%), dragging (100%)
 *   - Optional glow effect on drag
 *
 * Inherits from JUCE's ResizableCornerComponent for proper resize handling,
 * but overrides paint() for custom visual appearance.
 *
 * Reusable across all Bellweather Studios plugins.
 * Thread Safety: UI thread only.
 * =============================================================================
 */

#include <bw_ui/rendering/IRenderer.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace bws::ui
{

/**
 * UiResizeCorner - Custom resize corner with concentric arcs design.
 *
 * Inherits JUCE's ResizableCornerComponent for reliable resize behavior,
 * but provides custom visual styling:
 * - Concentric arcs (radar/weather station aesthetic)
 * - Brass gold color matching Bellweather theme
 * - Two states: idle (dim), dragging (bright)
 * - Glow effect on drag
 */
class UiResizeCorner : public juce::ResizableCornerComponent
{
public:
    /**
     * Create resize corner.
     * @param componentToResize The component this corner will resize
     * @param constrainer Bounds constrainer (min/max size, aspect ratio)
     */
    UiResizeCorner(juce::Component* componentToResize, juce::ComponentBoundsConstrainer* constrainer);

    ~UiResizeCorner() override = default;

    /** Set arc color (default: brass gold `#D4AF37`) */
    void setArcColour(juce::Colour colour);
    juce::Colour getArcColour() const { return arcColour_; }

    /** Set corner size - updates the component size */
    void setCornerSize(int size);
    int getCornerSize() const { return cornerSize_; }

    /** Enable/disable glow effect on drag */
    void setGlowEnabled(bool enabled);
    bool isGlowEnabled() const { return glowEnabled_; }

    // --- juce::Component overrides ---
    void paint(juce::Graphics& g) override;

private:
    /** Draw concentric arcs emanating from corner (IRenderer path: arcTo + strokePath) */
    void drawConcentricArcs(rendering::IRenderer& r, float centerX, float centerY, float alpha);

    juce::Colour arcColour_;
    int cornerSize_ = 20;
    bool glowEnabled_ = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UiResizeCorner)
};

} // namespace bws::ui
