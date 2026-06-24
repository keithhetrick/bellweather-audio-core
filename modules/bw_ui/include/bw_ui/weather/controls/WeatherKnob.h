// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

/**
 * =============================================================================
 * WeatherKnob.h - Enhanced Knob with Smart Text Parsing
 * =============================================================================
 *
 * Bellweather Studios - Weather Instrument Design System
 *
 * Extends UiArcKnob with FabFilter-style features:
 *   - Smart text parsing:
 *     - "1k" -> 1000 Hz
 *     - "10k" -> 10000 Hz
 *     - "A4" -> 440 Hz (musical note)
 *     - "C#3" -> note frequency
 *     - "C#3+13" -> note + cents offset
 *     - "2x" -> +6 dB (gain doubling)
 *     - "-3x" -> -9.5 dB (attenuation)
 *     - "50%" -> middle of range
 *   - Keyboard shortcuts:
 *     - Cmd/Ctrl+Click -> Reset to default
 *     - Shift+Drag -> Fine-tune (from UiArcKnob)
 *     - Alt+Drag -> Constrain to axis
 *   - Velocity-sensitive dragging
 *   - Hover tooltip with current value
 *
 * Usage:
 *   WeatherKnob freqKnob;
 *   freqKnob.enableFrequencyMode(true);  // Enable "1k" -> 1000 parsing
 *
 *   WeatherKnob gainKnob;
 *   gainKnob.enableGainMode(true);  // Enable "2x" -> +6dB parsing
 *
 * =============================================================================
 */

#include "bw_ui/Components/UiArcKnob.h"
#include <bw_ui/foundation/UiTheme.h>

namespace bws
{
namespace weather
{

/**
 * @brief Enhanced knob with smart text parsing and FabFilter-style features.
 *
 * Provides intelligent input parsing for frequency, gain, and percentage values,
 * along with professional-grade keyboard shortcuts and mouse interaction.
 */
class WeatherKnob
    : public bws::ui::UiArcKnob
    , private juce::Timer
{
public:
    WeatherKnob();
    ~WeatherKnob() override { stopTimer(); }

    // =========================================================================
    // Smart Text Parsing Modes
    // =========================================================================

    /**
     * Enable smart text parsing for frequency knobs.
     * Supports:
     *   - "1k" -> 1000 Hz
     *   - "10k" -> 10000 Hz
     *   - "A4" -> 440 Hz
     *   - "C#3" -> note frequency
     *   - "C#3+13" -> note + cents offset
     */
    void enableFrequencyMode(bool enable) { frequencyMode_ = enable; }
    bool isFrequencyMode() const { return frequencyMode_; }

    /**
     * Enable smart text parsing for gain knobs.
     * Supports:
     *   - "2x" -> +6.02 dB (doubling)
     *   - "4x" -> +12.04 dB (quadrupling)
     *   - "0.5x" -> -6.02 dB (halving)
     *   - "-6" -> -6 dB
     *   - "+3" -> +3 dB
     */
    void enableGainMode(bool enable) { gainMode_ = enable; }
    bool isGainMode() const { return gainMode_; }

    /**
     * Enable percentage parsing (always on by default).
     * Supports:
     *   - "50%" -> middle of range
     *   - "0%" -> minimum
     *   - "100%" -> maximum
     */
    void enablePercentageMode(bool enable) { percentageMode_ = enable; }
    bool isPercentageMode() const { return percentageMode_; }

    // =========================================================================
    // Velocity Sensitivity
    // =========================================================================

    /**
     * Enable velocity-sensitive dragging.
     * Slower mouse movement = finer adjustment.
     */
    void setVelocitySensitive(bool enable) { velocitySensitive_ = enable; }
    bool isVelocitySensitive() const { return velocitySensitive_; }

    // =========================================================================
    // Musical Note Support
    // =========================================================================

    /**
     * Set the reference frequency for A4 (default: 440 Hz).
     * Used when parsing musical note names like "A4", "C#3", etc.
     */
    void setA4Frequency(double freq) { a4Frequency_ = freq; }
    double getA4Frequency() const { return a4Frequency_; }

    // =========================================================================
    // juce::Slider Overrides
    // =========================================================================

    double getValueFromText(const juce::String& text) override;
    juce::String getTextFromValue(double value) override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void paint(juce::Graphics& g) override;

    // =========================================================================
    // FabFilter-Style Interactions
    // =========================================================================

    /** Show right-click context menu with reset, enter value, MIDI learn options */
    void showContextMenu();

    /** Apply resolved theme (colors, spacing, typography). */
    void setTheme(const bws::ui::UiThemeResolved& t) { theme_ = &t; }

private:
    /** Update tooltip text with current parameter value */
    void updateTooltipText();
    // =========================================================================
    // Parsing Helpers
    // =========================================================================

    /** Parse frequency text (handles "1k", "A4", etc.) */
    double parseFrequencyText(const juce::String& text) const;

    /** Parse gain text (handles "2x", "-6", etc.) */
    double parseGainText(const juce::String& text) const;

    /** Parse percentage text (handles "50%", etc.) */
    double parsePercentageText(const juce::String& text) const;

    /** Check if text represents a musical note (A4, C#3, etc.) */
    static bool isMusicalNote(const juce::String& text);

    /** Convert musical note to frequency (A4=440Hz standard) */
    double noteToFrequency(const juce::String& noteText) const;

    /** Parse note name to semitone offset from A4 */
    static int parseNoteToSemitone(const juce::String& noteText);

    /** Extract cents offset from note text (e.g., "C#3+13" -> 13) */
    static double parseCentsOffset(const juce::String& noteText);

    // =========================================================================
    // Timer Callback (for reset feedback)
    // =========================================================================

    void timerCallback() override
    {
        isShowingResetFeedback_ = false;
        repaint();
        stopTimer();
    }

    // =========================================================================
    // State
    // =========================================================================

    bool frequencyMode_ = false;
    bool gainMode_ = false;
    bool percentageMode_ = true; // Always enabled by default
    bool velocitySensitive_ = false;
    double a4Frequency_ = 440.0;

    // Velocity tracking for velocity-sensitive mode
    juce::Point<int> lastDragPos_;
    juce::Time lastDragTime_;

    // Reset feedback (brief golden glow on Cmd+click reset)
    bool isShowingResetFeedback_ = false;

    const bws::ui::UiThemeResolved* theme_ = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WeatherKnob)
};

} // namespace weather
} // namespace bws
