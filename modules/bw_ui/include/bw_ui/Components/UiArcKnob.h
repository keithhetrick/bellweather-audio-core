// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

/**
 * =============================================================================
 * UiArcKnob.h - Professional arc-style knob with centered digital readout
 * =============================================================================
 *
 * Inspired by Native Instruments / modern plugin UIs.
 * Clean, minimal design with:
 *   - Large centered value display
 *   - Arc indicator showing position
 *   - Brass/gold accent color (matches Barometer theme)
 *   - No confusing tick labels
 *
 * Thread Safety: UI thread only - no RT concerns.
 * =============================================================================
 */

#include <bw_ui/rendering/IRenderer.h>
#include <bw_ui/interaction/DoubleClickAction.h>
#include <bw_ui/interaction/InteractionPolicy.h>
#include <bw_ui/kernel/Knob.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <functional>

namespace bws::ui
{

/**
 * UiArcKnob - Professional arc-style knob with centered digital readout.
 *
 * Features:
 * - Large centered value display with custom formatting
 * - Arc indicator showing current position
 * - Brass/gold accent color (configurable)
 * - Optional tick mark at 0 dB
 * - Smooth mouse interaction with fine-adjustment mode (Shift)
 * - Cmd/Ctrl+drag for secondary parameter (e.g., Output Trim) - FabFilter-style
 * - Double-click to reset to default value
 */
class UiArcKnob : public juce::Slider
{
public:
    UiArcKnob();
    ~UiArcKnob() override = default;

    void setKnobRole(UiKnobRole role);
    UiKnobRole getKnobRole() const noexcept { return knobRole_; }
    const kernel::ResolvedKnobModel& getResolvedKnobModel() const noexcept { return resolvedModel_; }

    // --- Value Display ---

    /** Set format function for value display (e.g., "+0.0 dB", "-inf dB") */
    void setValueDisplayFormat(std::function<juce::String(double)> formatter);

    /** Set font size for the centered value display (default: 28.0f) */
    void setValueFontSize(float size)
    {
        valueFontSize_ = size;
        repaint();
    }

    // --- Secondary Parameter (Trim Mode) ---

    /** Callback for Shift+drag trim value changes (normalized 0-1) */
    std::function<void(float)> onTrimValueChange;

    /** Callback to get current trim value for display (returns dB value) */
    std::function<float()> getTrimValue;

    /** Callback when trim drag begins (for DAW automation gesture start) */
    std::function<void()> onTrimDragStart;

    /** Callback when trim drag ends (for DAW automation gesture end) */
    std::function<void()> onTrimDragEnd;

    /** Set trim display format (e.g., "TRIM: +3.0 dB") */
    void setTrimDisplayFormat(std::function<juce::String(double)> formatter);

    /** Check if currently in trim mode */
    bool isInTrimMode() const { return isTrimMode_; }
    bool hasConfiguredTrimCapability() const noexcept;

    /** Check if currently in fine-tune mode (Cmd+drag) */
    bool isFineTuneMode() const { return isFineTuneMode_; }

    /** Test-only hook: force fine-tune state for deterministic GUI tests. */
    void setFineTuneModeForTesting(bool enabled);

    // --- Appearance ---

    /** Set arc color (default: brass gold 0xFFD4AF37) */
    void setArcColour(juce::Colour colour)
    {
        arcColour_ = colour;
        repaint();
    }
    juce::Colour getArcColour() const { return arcColour_; }

    /** Set background gradient colors */
    void setBackgroundColours(juce::Colour top, juce::Colour bottom);

    /** Set outer ring/border color */
    void setRingColour(juce::Colour colour)
    {
        ringColour_ = colour;
        repaint();
    }

    /** Enable/disable subtle tick mark at 0 dB (or custom position) */
    void setShowTickMark(bool show)
    {
        showTickMark_ = show;
        repaint();
    }
    bool getShowTickMark() const { return showTickMark_; }

    /** Set the value where the tick mark appears (default: 0.0 for 0 dB) */
    void setTickMarkValue(double value)
    {
        tickMarkValue_ = value;
        repaint();
    }

    /** Set arc thickness (default: 6.0f) */
    void setArcThickness(float thickness)
    {
        arcThickness_ = thickness;
        repaint();
    }

    /** Enable/disable glow effect on hover */
    void setShowHoverGlow(bool show)
    {
        showHoverGlow_ = show;
        repaint();
    }

    /** Locked controls remain hoverable but ignore all edit actions. */
    void setLocked(bool locked);
    bool isLocked() const { return locked_; }

    /** Cancel any active drag/edit interaction without mutating value. */
    void cancelUserInteraction();

    // --- Text Entry Mode ---

    /** Show text entry popup for direct value input (main gain) */
    void showTextEntry();

    /** Show text entry popup for trim value input (Shift+double-click) */
    void showTrimTextEntry();

    /** Callback when text entry is requested (allows parent to handle) */
    std::function<void()> onTextEntryRequested;

    /** Callback when trim text entry is requested (allows parent to handle) */
    std::function<void()> onTrimTextEntryRequested;

    // --- Context Menu ---

    /** Callback for "Set as Default" menu action */
    std::function<void(float)> onSetAsDefault;

    /** Custom default value for reset (0.0 dB = 0.5 normalized for -96 to +24 range) */
    void setCustomDefaultValue(double value)
    {
        customDefaultValue_ = value;
        hasCustomDefault_ = true;
    }

    using DoubleClickAction = bws::ui::DoubleClickAction;
    using InteractionPolicy = bws::ui::interaction::InteractionPolicy;

    // Message-thread only. Custom callback must outlive this knob and must
    // not synchronously destroy it.
    void setDoubleClickAction(DoubleClickAction action);
    void setCustomDoubleClickCallback(std::function<void()> callback);
    DoubleClickAction getDoubleClickAction() const noexcept { return policy_.doubleClick.action; }

    void setInteractionPolicy(InteractionPolicy newPolicy);
    const InteractionPolicy& getInteractionPolicy() const noexcept { return policy_; }

    /** Get the current value in the slider's range (not normalized) */
    double getValueInRange() const { return getValue(); }

    /** Set value from range value (not normalized) */
    void setValueFromRange(double rangeValue) { setValue(rangeValue); }

    // --- juce::Slider overrides ---
    void paint(juce::Graphics& g) override;
    bool hitTest(int x, int y) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    void modifierKeysChanged(const juce::ModifierKeys& modifiers) override;
    void visibilityChanged() override;
    void parentHierarchyChanged() override;
    bool keyPressed(const juce::KeyPress& key) override;

private:
    /** Draw an arc segment (IRenderer path: arcTo + strokePath) */
    void drawArc(rendering::IRenderer& r, float startAngle, float endAngle, float radius, float thickness,
                 juce::Colour colour);

    /** Default value formatter: shows +0.0 dB, -inf for <= -96 */
    static juce::String defaultValueFormatter(double value);

    /** Parse user input for text entry (supports dB, multiplier "2x", percentage "50%") */
    double parseValueInput(const juce::String& input) const;

    /** Show context menu on right-click (forTrim=true shows trim-specific menu) */
    void showContextMenu(bool forTrim = false);

    /** Copy current value to system clipboard */
    void copyValueToClipboard();

    /** Paste value from system clipboard */
    void pasteValueFromClipboard();

    /** Create and show inline text editor over the value display */
    void createInlineEditor(bool forTrim);

    /** Handle inline editor completion */
    void handleInlineEditorResult(const juce::String& text, bool forTrim);

    // Inline text editor (appears over value display)
    std::unique_ptr<juce::TextEditor> inlineEditor_;
    bool inlineEditorIsForTrim_ = false;

    // Value display
    std::function<juce::String(double)> valueFormatter_;
    float valueFontSize_ = 28.0f;
    UiKnobRole knobRole_ {UiKnobRole::Hero};
    kernel::ResolvedKnobModel resolvedModel_ {kernel::resolveKnobModel(UiKnobRole::Hero)};

    // Appearance
    juce::Colour arcColour_ {0xFFD4AF37};      // Brass gold
    juce::Colour bgTopColour_ {0xFF2A2A2A};    // Dark gradient top
    juce::Colour bgBottomColour_ {0xFF1A1A1A}; // Dark gradient bottom
    juce::Colour ringColour_ {0xFF3A3A3A};     // Outer ring
    juce::Colour dimArcColour_ {0xFF3A3A3A};   // Background arc (dim)

    bool showTickMark_ = false;
    double tickMarkValue_ = 0.0; // Value where tick mark appears (e.g., 0 dB)

    float arcThickness_ = 6.0f;
    bool showHoverGlow_ = true;
    bool locked_ = false;

    // Fine-tune mode tracking (for dynamic Shift toggle mid-drag)
    bool isFineTuneMode_ = false;

    // Trim mode (Shift+drag for secondary parameter)
    bool isTrimMode_ = false;
    bool isTrimPreviewMode_ = false; // Shift+hover shows trim preview
    float trimDragStartValue_ = 0.0f;
    int trimDragStartY_ = 0;
    std::function<juce::String(double)> trimFormatter_;

    // Gain drag tracking (for mid-drag mode switching)
    float gainDragStartValue_ = 0.0f;
    int gainDragStartY_ = 0;

    // Trim arc color (shown during trim adjustment)
    juce::Colour trimArcColour_ {0xFFFFCC00}; // Yellow to match trim text

    // Custom default value for reset
    double customDefaultValue_ = 0.0;
    bool hasCustomDefault_ = false;

    bws::ui::interaction::InteractionPolicy policy_ {};

    // Clipboard value storage key (for copy/paste between instances)
    static juce::String clipboardValueKey_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UiArcKnob)
};

} // namespace bws::ui
