// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include "bw_ui/adapters/UiThemeKernelAdapter.h"
#include "bw_ui/Components/ComponentSize.h"
#include "bw_ui/Components/UiLabel.h"
#include "bw_ui/Components/UiReadout.h"
#include "bw_ui/Components/ControlStackMetrics.h"

namespace bws::ui
{

class UiLabeledSlider : public juce::Component
{
public:
    explicit UiLabeledSlider(const UiThemeResolved& theme);
    ~UiLabeledSlider() override;

    // Size-variant factory methods
    static std::unique_ptr<UiLabeledSlider> xs(const UiThemeResolved& theme, const juce::String& label = "");
    static std::unique_ptr<UiLabeledSlider> sm(const UiThemeResolved& theme, const juce::String& label = "");
    static std::unique_ptr<UiLabeledSlider> md(const UiThemeResolved& theme, const juce::String& label = "");
    static std::unique_ptr<UiLabeledSlider> lg(const UiThemeResolved& theme, const juce::String& label = "");
    static std::unique_ptr<UiLabeledSlider> xl(const UiThemeResolved& theme, const juce::String& label = "");

    // Set size variant programmatically
    void setSizeVariant(ComponentSize size);
    ComponentSize getSizeVariant() const { return currentSizeVariant; }

    juce::Slider& slider() noexcept { return sliderComp; }

    void setLabelText(const juce::String& text);
    void setScale(float newScale);
    void setTheme(const UiThemeResolved& newTheme);
    void setReadoutRole(UiReadoutRole newRole);
    void setUnit(ReadoutUnit newUnit);
    void setFormatter(std::function<juce::String(double)> formatterFn);

    juce::Rectangle<int> debugReadoutBounds() const { return readout.getBounds(); }
    juce::Rectangle<int> debugSliderBounds() const { return sliderComp.getBounds(); }

    void resized() override;

#if BWS_TESTING
    juce::Rectangle<int> testLabelBounds() const { return label.getBounds(); }
    juce::Rectangle<int> testReadoutBounds() const { return readout.getBounds(); }
    juce::Rectangle<int> testSliderBounds() const { return sliderComp.getBounds(); }
#endif

private:
    class SliderLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        SliderLookAndFeel(const UiThemeResolved* themeIn, float* scaleRef);
        void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                              float minSliderPos, float maxSliderPos, juce::Slider::SliderStyle style,
                              juce::Slider& slider) override;
        int getSliderThumbRadius(juce::Slider&) override;

    private:
        const UiThemeResolved* theme {nullptr};
        float* scalePtr {nullptr};
    };

    void refreshReadout();
    void applySizeVariant(ComponentSize size);

    ComponentSize currentSizeVariant {ComponentSize::MD};
    UiThemeResolved theme;
    kernel::ThemeSnapshot kernelTheme;
    float scale {1.0f};
    UiKnobRole spacingRole {UiKnobRole::Primary};
    UiReadoutRole readoutRole {UiReadoutRole::Compact};
    UiLabel label;
    UiReadout readout;
    juce::Slider sliderComp;
    SliderLookAndFeel sliderLnf;
    ReadoutUnit unit {ReadoutUnit::Plain};
    std::function<juce::String(double)> formatter;
};

} // namespace bws::ui
