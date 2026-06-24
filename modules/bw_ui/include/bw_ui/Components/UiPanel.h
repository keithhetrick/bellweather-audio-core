// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "bw_ui/foundation/UiTheme.h"
#include "bw_ui/Components/UiKnob.h" // For ComponentSize enum

namespace bws::ui
{

class UiPanel : public juce::Component
{
public:
    enum class Treatment
    {
        Standard,
        EmbeddedBed
    };

    explicit UiPanel(const UiThemeResolved& themeIn);

    // Size-variant factory methods
    static std::unique_ptr<UiPanel> xs(const UiThemeResolved& theme);
    static std::unique_ptr<UiPanel> sm(const UiThemeResolved& theme);
    static std::unique_ptr<UiPanel> md(const UiThemeResolved& theme);
    static std::unique_ptr<UiPanel> lg(const UiThemeResolved& theme);
    static std::unique_ptr<UiPanel> xl(const UiThemeResolved& theme);

    // Set size variant programmatically
    void setSizeVariant(ComponentSize size);
    ComponentSize getSizeVariant() const { return currentSizeVariant; }

    void setTheme(const UiThemeResolved& newTheme);
    void setScale(float newScale);
    void setFramed(bool shouldFrame);
    void setTreatment(Treatment newTreatment);

    void paint(juce::Graphics& g) override;

private:
    void paintMatte(juce::Graphics& g, juce::Rectangle<float> bounds) const;
    void paintBrushed(juce::Graphics& g, juce::Rectangle<float> bounds) const;
    float framePadding() const;
    void applySizeVariant(ComponentSize size);

    ComponentSize currentSizeVariant {ComponentSize::MD};
    UiThemeResolved theme;
    float scale {1.0f};
    bool framed {false};
    Treatment treatment {Treatment::Standard};
};

} // namespace bws::ui
