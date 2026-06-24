// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

/**
 * =============================================================================
 * WeatherSegmentedControl.h - Segmented Mode Selector Component
 * =============================================================================
 *
 * Bellweather Studios - Weather Instrument Design System
 *
 * Horizontal segmented control for mode selection (iOS/macOS style).
 * Visual: Horizontal bar with segments, selected segment highlighted.
 * Follows Brass design system colors and radii.
 *
 * Usage:
 *   WeatherSegmentedControl blendMode;
 *   blendMode.addSegment("SHIFT");      // Index 0
 *   blendMode.addSegment("XFADE");      // Index 1
 *   blendMode.setSelectedIndex(0);
 *   blendMode.onSelectionChanged = [&](int index) { ... };
 *   addAndMakeVisible(blendMode);
 *
 * =============================================================================
 */

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "bw_ui/adapters/UiThemeKernelAdapter.h"
#include "bw_ui/foundation/UiTheme.h"
#include <vector>
#include <functional>
#include <atomic>

namespace bws::weather
{

/**
 * @brief Segmented control for mode selection (iOS/macOS style).
 *
 * Visual: Horizontal bar with segments, selected segment highlighted.
 * Follows Brass design system colors and radii.
 */
class WeatherSegmentedControl : public juce::Component
{
public:
    WeatherSegmentedControl();
    ~WeatherSegmentedControl() override = default;

    /**
     * @brief Add a segment with label.
     * @param label Segment label text
     */
    void addSegment(const juce::String& label);

    /**
     * @brief Remove all segments.
     */
    void clearSegments();

    /**
     * @brief Get currently selected segment index.
     * @return Selected index (0-based)
     */
    int getSelectedIndex() const { return selectedIndex_; }

    /**
     * @brief Set selected segment.
     * @param index Segment index to select
     * @param notify If true, triggers onSelectionChanged callback
     */
    void setSelectedIndex(int index, bool notify = true);

    /**
     * @brief Set scale factor for HiDPI rendering.
     * @param scale Scale multiplier (1.0 = 100%)
     */
    void setScaleFactor(float scale);

    /** Set the resolved UI theme for typography. */
    void setTheme(const bws::ui::UiThemeResolved& t);

    /** Callback when selection changes */
    std::function<void(int)> onSelectionChanged;

    // juce::Component overrides
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    bool keyPressed(const juce::KeyPress& key) override;

private:
    /**
     * @brief Get segment index at mouse position.
     * @param pos Mouse position in component coordinates
     * @return Segment index or -1 if outside
     */
    int getSegmentIndexAt(juce::Point<int> pos) const;

    std::vector<juce::String> segments_;
    int selectedIndex_ = 0;
    int pressedIndex_ = -1;
    int hoveredIndex_ = -1;
    float scaleFactor_ = 1.0f;

    const bws::ui::UiThemeResolved* theme_ = nullptr;
    bws::ui::kernel::ThemeSnapshot kernelTheme_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WeatherSegmentedControl)
};

/**
 * @brief APVTS attachment for WeatherSegmentedControl.
 *
 * Bridges segmented control to AudioParameterChoice.
 * Provides two-way sync between UI and parameter with gesture support.
 */
class SegmentedControlAttachment : private juce::AudioProcessorValueTreeState::Listener
{
public:
    /**
     * @brief Create attachment between APVTS parameter and control.
     * @param apvts AudioProcessorValueTreeState reference
     * @param paramID Parameter ID to attach
     * @param control WeatherSegmentedControl to sync
     */
    SegmentedControlAttachment(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID,
                               WeatherSegmentedControl& control);

    ~SegmentedControlAttachment() override;

private:
    void parameterChanged(const juce::String&, float newValue) override;

    juce::AudioProcessorValueTreeState& apvts_;
    juce::String paramID_;
    WeatherSegmentedControl& control_;
    std::atomic<bool> ignoreCallback_ {false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SegmentedControlAttachment)
};

} // namespace bws::weather
