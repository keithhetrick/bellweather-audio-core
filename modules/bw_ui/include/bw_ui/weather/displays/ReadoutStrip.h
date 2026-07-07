// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

/**
 * @file ReadoutStrip.h
 * @brief Compact horizontal readout strip for Weather Instrument Design System
 *
 * DESIGN GOALS:
 * - Height: 34px (Fibonacci)
 * - Shows live parameter values in compact format
 * - Reveal button to show/hide iris aperture
 * - Brass aesthetic consistent with Weather Instrument suite
 *
 * USAGE:
 * @code
 * auto readout = std::make_unique<ReadoutStrip>(apvts);
 * readout->addParameter("gain", "GAIN", "dB");
 * readout->addParameter("width", "WIDTH", "%");
 * readout->addParameter("mix", "MIX", "%");
 * readout->onRevealClicked = [this]() { toggleIrisVisibility(); };
 * @endcode
 */

#include "bw_ui/foundation/UiTheme.h"
#include "bw_ui/adapters/JuceParameterAdapter.h"
#include "bw_ui/interaction/DoubleClickAction.h"
#include "bw_ui/interaction/InteractionPolicy.h"
#include "bw_ui/interaction/ValueInteraction.h"
#include "bw_ui/kernel/ModSet.h"
#include "bw_ui/kernel/ReadoutValue.h"
#include "bw_ui/kernel/State.h"
#include "bw_ui/kernel/Theme.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <optional>
#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

namespace bws::weather
{

/**
 * @class ReadoutStrip
 * @brief Compact horizontal strip showing live parameter values with reveal button
 *
 * LAYOUT:
 * [GAIN: 0dB] [WIDTH: 100%] [MIX: 0%] ─────────── [▼ ADVANCED]
 *
 * FEATURES:
 * - 34px height (Fibonacci, between 21 and 55)
 * - Updates at 30Hz for smooth value display
 * - Brass theme colors with TEXT_SECONDARY for values
 * - Button reveals/hides iris aperture
 * - Left-to-right layout: readouts (70%) + button (30%)
 * - INTERACTIVE: Drag/scroll on values to adjust parameters (pro-level feature)
 */
class ReadoutStrip
    : public juce::Component
    , public juce::SettableTooltipClient
    , private juce::Timer
{
public:
    //==========================================================================
    // CONSTRUCTION
    //==========================================================================

    explicit ReadoutStrip(juce::AudioProcessorValueTreeState& apvts);
    ~ReadoutStrip() override;

    //==========================================================================
    // CONFIGURATION
    //==========================================================================

    /**
     * @brief Add a parameter to display in the strip
     * @param paramId Parameter ID in APVTS
     * @param displayName Display name (e.g., "GAIN")
     * @param spec How to format the value (canonical kernel formatter)
     * @param displayScale Multiplier from the param's real value to display units
     *        (e.g. 100 for a 0..1 param shown as a percent)
     */
    void addParameter(const juce::String& paramId, const juce::String& displayName, bws::ui::kernel::FormatSpec spec,
                      double displayScale = 1.0);

    /**
     * @brief Clear all parameter displays
     */
    void clearParameters();

    /**
     * @brief Set reveal button text
     * @param text Button text (e.g., "▼ ADVANCED" or "▲ HIDE")
     */
    void setRevealButtonText(const juce::String& text);

    /**
     * @brief Show/hide the reveal button (hide when Tab 3 replaces ADVANCED)
     */
    void setRevealButtonVisible(bool visible);

    /**
     * @brief Set resolved theme for typography (optional; falls back to default)
     * @param t Pointer to a long-lived UiThemeResolved
     */
    void setTheme(const bws::ui::UiThemeResolved& t);

    /**
     * @brief Set scale factor for HiDPI support
     * @param scale Scale factor (1.0 = 100%)
     */
    void setScale(float scale);
    void setTextScaleMultiplier(float multiplier);

    /**
     * @brief Get current scale factor
     * @return Current scale factor
     */
    float getScale() const { return scale_; }

    /**
     * @brief Get the strip height (34px, scaled)
     * @return Strip height in pixels
     */
    int getStripHeight() const;

    //==========================================================================
    // CALLBACKS
    //==========================================================================

    /**
     * @brief Callback when reveal button is clicked
     */
    std::function<void()> onRevealClicked;

    //==========================================================================
    // INTERACTIVE READOUTS (Pro-Level Feature)
    //==========================================================================

    /**
     * @brief Enable/disable interactive drag-to-adjust on readout values
     * @param enable True to enable dragging (default: true)
     *
     * When enabled, users can drag horizontally or scroll on any readout
     * value to adjust the parameter directly from the collapsed strip.
     */
    void setInteractiveReadouts(bool enable) { interactiveEnabled_ = enable; }

    /**
     * @brief Check if interactive readouts are enabled
     * @return True if dragging/scrolling is enabled
     */
    bool isInteractiveEnabled() const { return interactiveEnabled_; }

    /** Lock/unlock a specific readout segment while keeping hover affordances. */
    void setItemLocked(int index, bool locked);

    /** Set per-item tooltip text owned by caller/editor. */
    void setItemTooltip(int index, const juce::String& tooltip);

    /** Override the displayed value of a segment with a caller-supplied real
        (denormalised, pre-displayScale) value; pass std::nullopt to fall back to
        the live APVTS value. Used when a parameter's effective value differs from
        its raw value (e.g. an AUTO-driven cutoff). */
    void setItemDisplayOverride(int index, std::optional<float> realValue);

    /** ForTesting: the formatted text a segment currently renders. */
    juce::String getItemValueTextForTesting(int index) const;

    /** ForTesting: drive a segment's gesture by index (headless, no MouseEvent).
        Each returns the resulting normalized parameter value. The characterization
        golden pins these across the ValueInteraction migration. */
    float dragSegmentForTesting(int index, int pixelsUp, bws::ui::kernel::ModSet mods);
    float wheelSegmentForTesting(int index, float deltaY, bws::ui::kernel::ModSet mods);
    float doubleClickSegmentForTesting(int index);

    /** Enable a magnetic snap during drag at a normalised position. */
    void setItemCenterDetent(int index, bool enable, float position = 0.5f, float snapRadius = 0.02f);

    /** Cancel active drag gesture without emitting edits. */
    void cancelInteraction();

    using DoubleClickAction = bws::ui::DoubleClickAction;
    using InteractionPolicy = bws::ui::interaction::InteractionPolicy;

    // Strip-level. Applies to all parameters that pass the interactiveEnabled_
    // + isReadoutEditable gates. Custom callback must outlive this strip and
    // must not synchronously destroy it. Message-thread only.
    void setDoubleClickAction(DoubleClickAction action);
    void setCustomDoubleClickCallback(std::function<void()> callback);
    DoubleClickAction getDoubleClickAction() const noexcept { return policy_.doubleClick.action; }

    // Reset uses the parameter under the cursor (per-item), not policy.reset.
    void setInteractionPolicy(InteractionPolicy newPolicy);
    const InteractionPolicy& getInteractionPolicy() const noexcept { return policy_; }

    //==========================================================================
    // COMPONENT OVERRIDES
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void visibilityChanged() override;
    void parentHierarchyChanged() override;

private:
    //==========================================================================
    // INTERNAL TYPES
    //==========================================================================

    struct ParameterDisplay
    {
        juce::String paramId;
        juce::String displayName;
        bws::ui::kernel::FormatSpec spec;
        double displayScale = 1.0;
        juce::RangedAudioParameter* param = nullptr;
        float lastValue = 0.0f;        // For change detection
        juce::Rectangle<float> bounds; // Cached hit-test bounds for interaction
        bws::ui::kernel::AvailabilityState availability = bws::ui::kernel::AvailabilityState::Enabled;
        juce::String tooltip;
        std::optional<float> displayOverride; // real units; when set, shown instead of the live param value

        // Per-segment action model: JUCE pinned to the adapter; the gesture
        // orchestration (drag/wheel) is the native, JUCE-free ValueInteraction.
        // unique_ptr keeps the adapter address stable across vector reallocation,
        // so the model's IValueParameter& stays valid.
        std::unique_ptr<bws::ui::adapters::JuceParameterAdapter> adapter;
        std::unique_ptr<bws::ui::interaction::ValueInteraction> vi;
    };

    // Formatted value text for a segment (honours displayOverride when set).
    juce::String formatItemValue(const ParameterDisplay& display) const;

    //==========================================================================
    // TIMER CALLBACK
    //==========================================================================

    void timerCallback() override;

    //==========================================================================
    // DRAWING HELPERS
    //==========================================================================

    void paintBackground(juce::Graphics& g, juce::Rectangle<float> bounds);
    void paintParameterValues(juce::Graphics& g, juce::Rectangle<float> bounds);

    // Recompute cachedMaxPairWidth_ from the current parameters_ + font.
    // Call whenever something that affects label/value text width changes
    // (parameters_, scale_, theme, textScaleMultiplier_) - NOT per paint.
    void recomputeMaxPairWidth();

    //==========================================================================
    // INTERACTION HELPERS
    //==========================================================================

    /**
     * @brief Find which parameter (if any) is at the given point
     * @param point Mouse position in component coordinates
     * @return Index of parameter, or -1 if none
     */
    int getParameterIndexAt(juce::Point<float> point) const;

    /**
     * @brief Update the hovered parameter and cursor
     * @param index Parameter index to highlight (-1 for none)
     */
    void setHoveredParameter(int index);

    //==========================================================================
    // MEMBER VARIABLES
    //==========================================================================

    juce::AudioProcessorValueTreeState& apvts_;
    std::vector<ParameterDisplay> parameters_;

    // Widest "label: value" pair across parameters_, in pixels at the
    // current font (scale_ * textScaleMultiplier_). Value text is measured
    // at each param's normalized 0.0 and 1.0 endpoints, so the cache is
    // value-independent - does not change as the user drags a knob.
    // See recomputeMaxPairWidth() for invalidation triggers.
    float cachedMaxPairWidth_ = 0.0f;

    // Button
    std::unique_ptr<juce::TextButton> revealButton_;

    // Theme
    const bws::ui::UiThemeResolved* theme_ = nullptr;
    bws::ui::kernel::ThemeSnapshot kernelTheme_;

    // State
    float scale_ = 1.0f;
    float textScaleMultiplier_ = 1.0f;
    bool needsRepaint_ = false;

    // Interactive state
    bool interactiveEnabled_ = true;
    int hoveredParameterIndex_ = -1;
    int draggingParameterIndex_ = -1; // active drag segment; -1 = none
    int dragStartY_ = 0;              // pointer anchor; model captures the value anchor

    bws::ui::interaction::InteractionPolicy policy_ {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReadoutStrip)
};

} // namespace bws::weather
