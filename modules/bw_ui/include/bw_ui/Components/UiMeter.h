// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "bw_ui/foundation/UiTheme.h"

// Meter semantics: primary value is recommended for PEAK; optional secondary value is recommended for RMS.
// Performance: repaint only on value/theme/scale/orientation changes; no layout invalidation or per-frame text.
// If peak-hold is added later, use a shared tick source (no per-instance timers).

namespace bws::ui
{

class UiMeter : public juce::Component
{
public:
    enum class Orientation
    {
        Horizontal,
        Vertical
    };

    UiMeter(const UiThemeResolved& themeIn, Orientation orientationIn = Orientation::Vertical);

    void setTheme(const UiThemeResolved& newTheme);
    void setScale(float newScale);
    void setOrientation(Orientation o);
    void setValue(float normalized01);
    void setSecondaryValue(float normalized01);
    void setDisabled(bool shouldDisable);

    void paint(juce::Graphics& g) override;

private:
    UiThemeResolved theme;
    float scale {1.0f};
    Orientation orientation {Orientation::Vertical};
    float value {0.0f};
    float secondary {-1.0f}; // <0 means unused
    bool disabled {false};

    float padding() const;
    float radius() const;
    juce::Rectangle<float> trackBounds() const;
    juce::Rectangle<float> levelBounds(const juce::Rectangle<float>& track, float v) const;
};

} // namespace bws::ui
