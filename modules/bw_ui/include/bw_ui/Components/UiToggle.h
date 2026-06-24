// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include "bw_ui/adapters/UiThemeKernelAdapter.h"
#include "bw_ui/Components/UiLabel.h"
#include "bw_ui/kernel/State.h"
#include "bw_ui/foundation/UiTheme.h"

namespace bws::ui
{

class UiToggle : public juce::Component
{
public:
    enum class Size
    {
        Default,
        Dense
    };

    UiToggle(const UiThemeResolved& themeIn, Size sizeIn = Size::Default);

    void setTheme(const UiThemeResolved& newTheme);
    void setScale(float newScale);
    void setValue(bool newValue);
    bool getValue() const noexcept { return value; }
    void setOnChange(std::function<void(bool)> handler);
    void setLabel(const juce::String& text);
    void setDisabled(bool shouldDisable);
    void setSize(Size newSize);

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseEnter(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    bool keyPressed(const juce::KeyPress& key) override;
    void focusGained(FocusChangeType) override;
    void focusLost(FocusChangeType) override;

private:
    struct PaintSpec
    {
        juce::Colour track;
        juce::Colour outline;
        juce::Colour thumb;
    };

    PaintSpec currentSpec() const;
    juce::Rectangle<float> trackBounds() const;
    juce::Rectangle<float> thumbBounds(const juce::Rectangle<float>& track) const;
    float trackHeight() const;
    float trackWidth() const;
    float cornerRadius() const;
    void updatePressedState(bool shouldPress);
    void updateHoverState(bool isHovered);
    void commitToggleChange(bool newValue, bool fromUserInteraction);

    UiThemeResolved theme;
    kernel::ThemeSnapshot kernelTheme;
    UiLabel label;
    juce::String labelText;
    Size size {Size::Default};
    float scale {1.0f};
    bool value {false};
    kernel::ControlState state;
    std::function<void(bool)> onChange;
    bool pointerDown_ {false};
};

} // namespace bws::ui
