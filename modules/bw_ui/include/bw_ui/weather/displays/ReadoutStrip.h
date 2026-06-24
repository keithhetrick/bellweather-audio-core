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
#include "bw_ui/adapters/GestureSession.h"
#include "bw_ui/adapters/JuceParameterAdapter.h"
#include "bw_ui/interaction/DoubleClickAction.h"
#include "bw_ui/interaction/InteractionPolicy.h"
#include "bw_ui/kernel/ParameterGestureScope.h"
#include "bw_ui/kernel/State.h"
#include "bw_ui/kernel/Theme.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
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
     * @param suffix Suffix for value (e.g., "%")
     * @param formatter Optional formatter mapping (normalized value, parameter) to display text
     */
    void addParameter(const juce::String& paramId, const juce::String& displayName, const juce::String& suffix = "",
                      std::function<juce::String(float normalizedValue, juce::RangedAudioParameter&)> formatter = {});

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
        juce::String suffix;
        juce::RangedAudioParameter* param = nullptr;
        float lastValue = 0.0f;        // For change detection
        juce::Rectangle<float> bounds; // Cached hit-test bounds for interaction
        bws::ui::kernel::AvailabilityState availability = bws::ui::kernel::AvailabilityState::Enabled;
        juce::String tooltip;
        std::function<juce::String(float normalizedValue, juce::RangedAudioParameter&)> formatter;
        bool centerDetentEnabled = false;
        float centerDetentPosition = 0.5f;
        float centerDetentSnapRadius = 0.02f;
    };

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

    /**
     * @brief Apply a delta change to a parameter
     * @param index Parameter index
     * @param delta Normalized delta (-1 to +1 range, will be scaled)
     * @param fineTune True for fine adjustment (shift held)
     */
    void adjustParameter(int index, float delta, bool fineTune);

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
    int draggingParameterIndex_ = -1;
    float dragStartValue_ = 0.0f;
    int dragStartY_ = 0;

    // unique_ptr indirection: GestureSession is non-movable (scope holds
    // IParameter* to &this->adapter), so optional<GestureSession> can't be
    // moved. unique_ptr is movable, enabling the linearize-then-destroy
    // pattern needed for re-entry safety.
    std::unique_ptr<bws::ui::adapters::GestureSession> slideGesture_;
    std::unique_ptr<bws::ui::adapters::GestureSession> wheelGesture_;
    double lastWheelGestureMs_ = 0.0; // message-thread only

    bws::ui::interaction::InteractionPolicy policy_ {};

    // Constants
    static constexpr int kDragSensitivity = 150;     // Pixels for full range drag
    static constexpr int kFineDragSensitivity = 600; // Pixels for fine drag (4x slower)
    static constexpr double kWheelGestureTimeoutMs = 180.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReadoutStrip)
};

} // namespace bws::weather
