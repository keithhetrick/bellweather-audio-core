// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "bw_ui/adapters/UiThemeKernelAdapter.h"
#include "bw_ui/Components/UiLabel.h"
#include "bw_ui/Components/ControlStackMetrics.h"

namespace bws::ui
{

class UiControlTile : public juce::Component
{
public:
    enum class ControlType
    {
        Knob,
        Slider
    };

    UiControlTile(ControlType typeIn, juce::Component& controlIn, const UiThemeResolved& themeIn,
                  UiKnobRole knobRoleIn = UiKnobRole::Primary, UiReadoutRole readoutRoleIn = UiReadoutRole::Standard);

    void setTheme(const UiThemeResolved& newTheme);
    void setScale(float newScale);
    void setLabelText(const juce::String& text);

    void resized() override;

    static int preferredKnobWidth(UiKnobRole role, float scale, const UiThemeResolved& theme);
    static int preferredKnobHeight(UiKnobRole role, float scale, const UiThemeResolved& theme,
                                   UiReadoutRole readoutRole = UiReadoutRole::Standard);
    static int preferredSliderHeight(float scale, const UiThemeResolved& theme,
                                     UiReadoutRole readoutRole = UiReadoutRole::Compact);

#if BWS_TESTING
    juce::Rectangle<int> testLabelBounds() const { return label.getBounds(); }
    juce::Rectangle<int> testControlBounds() const
    {
        return control != nullptr ? control->getBounds() : juce::Rectangle<int>();
    }
    UiReadoutRole testReadoutRole() const noexcept { return readoutRole; }
#endif

private:
    void refreshPadding();
    void applyScaleToControl();
    void applyThemeToControl();

    ControlType type;
    juce::Component* control {nullptr};
    UiThemeResolved theme;
    kernel::ThemeSnapshot kernelTheme;
    float scale {1.0f};
    UiKnobRole knobRole;
    UiReadoutRole readoutRole;
    UiLabel label;
    bool hasLabel {false};
    int paddingPx {0};
};

} // namespace bws::ui
