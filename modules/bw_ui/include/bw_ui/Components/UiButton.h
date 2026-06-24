// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include "bw_ui/adapters/UiThemeKernelAdapter.h"
#include "bw_ui/foundation/UiTheme.h"
#include "bw_ui/Components/UiKnob.h" // For ComponentSize enum
#include "bw_ui/kernel/State.h"

namespace bws::ui
{

class UiButton : public juce::Component
{
public:
    enum class Variant
    {
        Primary,
        Ghost,
        Text
    };

    enum class Size
    {
        Default,
        Dense
    };

    UiButton(const UiThemeResolved& themeIn, juce::String labelText = {}, Variant variantIn = Variant::Primary,
             Size sizeIn = Size::Default);

    // Size-variant factory methods
    static std::unique_ptr<UiButton> xs(const UiThemeResolved& theme, const juce::String& text,
                                        Variant variant = Variant::Primary);
    static std::unique_ptr<UiButton> sm(const UiThemeResolved& theme, const juce::String& text,
                                        Variant variant = Variant::Primary);
    static std::unique_ptr<UiButton> md(const UiThemeResolved& theme, const juce::String& text,
                                        Variant variant = Variant::Primary);
    static std::unique_ptr<UiButton> lg(const UiThemeResolved& theme, const juce::String& text,
                                        Variant variant = Variant::Primary);
    static std::unique_ptr<UiButton> xl(const UiThemeResolved& theme, const juce::String& text,
                                        Variant variant = Variant::Primary);

    // Set size variant programmatically
    void setSizeVariant(ComponentSize size);
    ComponentSize getSizeVariant() const { return currentSizeVariant; }

    void setTheme(const UiThemeResolved& newTheme);
    void setScale(float newScale);
    void setLabel(juce::String newLabel);
    void setVariant(Variant newVariant);
    void setSize(Size newSize);
    void setDisabled(bool shouldDisable);
    void setOnClick(std::function<void()> handler);

    void paint(juce::Graphics& g) override;

    void mouseEnter(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    bool keyPressed(const juce::KeyPress& key) override;
    void focusGained(FocusChangeType) override;
    void focusLost(FocusChangeType) override;

private:
    void applySizeVariant(ComponentSize size);
    void updatePressedState(bool shouldPress);
    void updateHoverState(bool isHovered);
    void commitIfPossible();

    ComponentSize currentSizeVariant {ComponentSize::MD};
    UiThemeResolved theme;
    kernel::ThemeSnapshot kernelTheme;
    juce::String label;
    Variant variant {Variant::Primary};
    Size size {Size::Default};
    std::function<void()> onClick;
    float scale {1.0f};
    kernel::ControlState state;
    bool pointerDown_ {false};

    struct PaintSpec
    {
        juce::Colour background;
        juce::Colour outline;
        juce::Colour text;
    };

    PaintSpec currentSpec() const;
    juce::Rectangle<int> contentBounds() const;
    juce::Font labelFont() const;
    float radius() const;
};

} // namespace bws::ui
