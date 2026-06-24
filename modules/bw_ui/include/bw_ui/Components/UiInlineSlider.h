// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include "bw_ui/foundation/UiTheme.h"
#include "bw_ui/adapters/UiThemeKernelAdapter.h"
#include "bw_ui/interaction/DoubleClickAction.h"
#include "bw_ui/interaction/InteractionPolicy.h"

namespace bws::ui
{

class UiInlineSlider : public juce::Component
{
public:
    enum class Emphasis
    {
        Supporting,
        Hero
    };

    explicit UiInlineSlider(const UiThemeResolved& theme);
    ~UiInlineSlider() override;

    juce::Slider& getSlider() noexcept { return slider; }
    const juce::Slider& getSlider() const noexcept { return slider; }

    void setLabelText(const juce::String& text);
    void setLabelVisible(bool shouldShow);
    void setFormatter(std::function<juce::String(double)> formatterFn);
    void setTheme(const UiThemeResolved& newTheme);
    void setScale(float newScale);
    void setDefaultValue(double defaultValue);
    void setEmphasis(Emphasis newEmphasis);
    void setModifierResetAction(std::function<void()> action);
    void setTextEntryAction(std::function<void()> action);

    using DoubleClickAction = bws::ui::DoubleClickAction;
    using InteractionPolicy = bws::ui::interaction::InteractionPolicy;

    // Message-thread only. Custom callback (if used) must outlive this slider
    // and must not synchronously destroy it.
    void setDoubleClickAction(DoubleClickAction action);
    void setCustomDoubleClickCallback(std::function<void()> callback);
    DoubleClickAction getDoubleClickAction() const noexcept;

    void setInteractionPolicy(InteractionPolicy newPolicy);
    const InteractionPolicy& getInteractionPolicy() const noexcept;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void enablementChanged() override;

private:
    class InputSlider : public juce::Slider
    {
    public:
        std::function<void()> onVisualStateChanged;

        bws::ui::interaction::InteractionPolicy policy_ {};

        void mouseDown(const juce::MouseEvent& e) override;
        void mouseDrag(const juce::MouseEvent& e) override;
        void mouseDoubleClick(const juce::MouseEvent& e) override;
        void mouseEnter(const juce::MouseEvent& e) override;
        void mouseExit(const juce::MouseEvent& e) override;
        void mouseMove(const juce::MouseEvent& e) override;
        void focusGained(FocusChangeType cause) override;
        void focusLost(FocusChangeType cause) override;

    protected:
        void valueChanged() override;
        void startedDragging() override;
        void stoppedDragging() override;
    };

    class NullLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        void drawLinearSlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider::SliderStyle,
                              juce::Slider&) override
        {}
        int getSliderThumbRadius(juce::Slider&) override { return 0; }
    };

    juce::String currentValueText() const;
    juce::Rectangle<float> trackBounds() const;
    juce::Rectangle<int> labelBounds() const;
    juce::Rectangle<int> compactLabelBounds() const;
    juce::Rectangle<int> sliderHitBounds() const;
    float cornerRadius() const;
    float trackHeight() const;
    float compactLabelHeight() const;

    UiThemeResolved theme;
    kernel::ThemeSnapshot kernelTheme;
    float scale {1.0f};
    Emphasis emphasis {Emphasis::Supporting};
    juce::String labelText;
    bool labelVisible {true};
    std::function<juce::String(double)> formatter;
    InputSlider slider;
    NullLookAndFeel sliderLookAndFeel;
};

} // namespace bws::ui
