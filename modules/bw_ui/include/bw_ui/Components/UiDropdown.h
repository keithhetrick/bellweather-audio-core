// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "bw_ui/Components/ThemedPopupMenuLookAndFeel.h"

namespace bws::ui
{

class UiDropdown : public juce::ComboBox
{
public:
    UiDropdown(const UiThemeResolved& theme, UiDropdownRole role = UiDropdownRole::Standard);
    ~UiDropdown() override;

    void setTheme(const UiThemeResolved& newTheme);
    void setRole(UiDropdownRole newRole);
    void setScale(float newScale);

private:
    // Popup drawing comes from ThemedPopupMenuLookAndFeel (parent).
    // DropdownLookAndFeel adds closed-box drawing + role-aware combo font.
    class DropdownLookAndFeel : public ThemedPopupMenuLookAndFeel
    {
    public:
        DropdownLookAndFeel(const UiThemeResolved& theme, UiDropdownRole role);

        void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown, int buttonX, int buttonY,
                          int buttonW, int buttonH, juce::ComboBox& box) override;
        void positionComboBoxText(juce::ComboBox& box, juce::Label& label) override;
        juce::Font getComboBoxFont(juce::ComboBox& box) override;
    };

    DropdownLookAndFeel lnf;
};

} // namespace bws::ui
